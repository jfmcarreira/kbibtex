/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "part.h"

#include <typeinfo>

#include <QLabel>
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QApplication>
#include <QLayout>
#include <QKeyEvent>
#include <QSignalMapper>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPointer>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryFile>

#include <KMessageBox> // FIXME deprecated
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <KActionMenu>
#include <KSelectAction>
#include <KToggleAction>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KRun>
#include <KPluginFactory>
#include <KIO/StatJob>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KJobWidgets>

#include "file.h"
#include "macro.h"
#include "preamble.h"
#include "comment.h"
#include "fileinfo.h"
#include "fileexporterbibtexoutput.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "fileimporterris.h"
#include "fileimporterbibutils.h"
#include "fileexporterris.h"
#include "fileexporterbibutils.h"
#include "fileimporterpdf.h"
#include "fileexporterps.h"
#include "fileexporterpdf.h"
#include "fileexporterrtf.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterxml.h"
#include "fileexporterxslt.h"
#include "models/filemodel.h"
#include "filesettingswidget.h"
#include "filterbar.h"
#include "findduplicatesui.h"
#include "lyx.h"
#include "preferences.h"
#include "settingscolorlabelwidget.h"
#include "settingsfileexporterpdfpswidget.h"
#include "findpdfui.h"
#include "valuelistmodel.h"
#include "clipboard.h"
#include "idsuggestions.h"
#include "fileview.h"
#include "browserextension.h"
#include "logging_parts.h"

K_PLUGIN_FACTORY(KBibTeXPartFactory, registerPlugin<KBibTeXPart>();)
// FIXME K_EXPORT_PLUGIN(KBibTeXPartFactory("KBibTeXPart"))

static const char RCFileName[] = "kbibtexpartui.rc";
static const int smEntry = 1;
static const int smComment = 2;
static const int smPreamble = 3;
static const int smMacro = 4;

class KBibTeXPart::KBibTeXPartPrivate
{
private:
    KBibTeXPart *p;
    KSharedConfigPtr config;

    /**
     * Modifies a given URL to become a "backup" filename/URL.
     * A backup level or 0 or less does not modify the URL.
     * A backup level of 1 appends a '~' (tilde) to the URL's filename.
     * A backup level of 2 or more appends '~N', where N is the level.
     * The provided URL will be modified in the process. It is assumed
     * that the URL is not yet a "backup URL".
     */
    void constructBackupUrl(const int level, QUrl &url) const {
        if (level <= 0)
            /// No modification
            return;
        else if (level == 1)
            /// Simply append '~' to the URL's filename
            url.setPath(url.path() + QStringLiteral("~"));
        else
            /// Append '~' followed by a number to the filename
            url.setPath(url.path() + QString(QStringLiteral("~%1")).arg(level));
    }

public:
    File *bibTeXFile;
    PartWidget *partWidget;
    FileModel *model;
    SortFilterFileModel *sortFilterProxyModel;
    QSignalMapper *signalMapperNewElement;
    QAction *editCutAction, *editDeleteAction, *editCopyAction, *editPasteAction, *editCopyReferencesAction, *elementEditAction, *elementViewDocumentAction, *fileSaveAction, *elementFindPDFAction, *entryApplyDefaultFormatString;
    QMenu *viewDocumentMenu;
    QSignalMapper *signalMapperViewDocument;
    QSet<QObject *> signalMapperViewDocumentSenders;
    bool isSaveAsOperation;
    LyX *lyx;
    FindDuplicatesUI *findDuplicatesUI;
    ColorLabelContextMenu *colorLabelContextMenu;
    QAction *colorLabelContextMenuAction;
    QFileSystemWatcher fileSystemWatcher;

    KBibTeXPartPrivate(QWidget *parentWidget, KBibTeXPart *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), bibTeXFile(NULL), model(NULL), sortFilterProxyModel(NULL), signalMapperNewElement(new QSignalMapper(parent)), viewDocumentMenu(new QMenu(i18n("View Document"), parent->widget())), signalMapperViewDocument(new QSignalMapper(parent)), isSaveAsOperation(false), fileSystemWatcher(p) {
        connect(signalMapperViewDocument, SIGNAL(mapped(QObject*)), p, SLOT(elementViewDocumentMenu(QObject*)));
        connect(&fileSystemWatcher, SIGNAL(fileChanged(QString)), p, SLOT(fileExternallyChange(QString)));

        partWidget = new PartWidget(parentWidget);
        partWidget->fileView()->setReadOnly(!p->isReadWrite());
        connect(partWidget->fileView(), SIGNAL(modified()), p, SLOT(setModified()));

        setupActions();
    }

    ~KBibTeXPartPrivate() {
        delete bibTeXFile;
        delete model;
        delete signalMapperNewElement;
        delete viewDocumentMenu;
        delete signalMapperViewDocument;
    }


    void setupActions()
    {
        /// "Save" action
        fileSaveAction = p->actionCollection()->addAction(KStandardAction::Save, p, SLOT(documentSave()));
        fileSaveAction->setEnabled(false);
        p->actionCollection()->addAction(KStandardAction::SaveAs, p, SLOT(documentSaveAs()));
        /// "Save copy as" action
        QAction *saveCopyAsAction = new QAction(QIcon::fromTheme("document-save"), i18n("Save Copy As..."), p);
        p->actionCollection()->addAction("file_save_copy_as", saveCopyAsAction);
        connect(saveCopyAsAction, &QAction::triggered, p, &KBibTeXPart::documentSaveCopyAs);

        /// Filter bar widget
        QAction *filterWidgetAction = new QAction(i18n("Filter"), p);
        p->actionCollection()->addAction("toolbar_filter_widget", filterWidgetAction);
        filterWidgetAction->setIcon(QIcon::fromTheme("view-filter"));
        p->actionCollection()->setDefaultShortcut(filterWidgetAction, Qt::CTRL + Qt::Key_F);
        connect(filterWidgetAction, SIGNAL(triggered()), partWidget->filterBar(), SLOT(setFocus()));
        partWidget->filterBar()->setPlaceholderText(i18n("Filter bibliographic entries (%1)", filterWidgetAction->shortcut().toString()));

        /// Actions for creating new elements (entries, macros, ...)
        KActionMenu *newElementAction = new KActionMenu(QIcon::fromTheme("address-book-new"), i18n("New element"), p);
        p->actionCollection()->addAction("element_new", newElementAction);
        QMenu *newElementMenu = new QMenu(newElementAction->text(), p->widget());
        newElementAction->setMenu(newElementMenu);
        connect(newElementAction, SIGNAL(triggered()), p, SLOT(newEntryTriggered()));
        QAction *newEntry = newElementMenu->addAction(QIcon::fromTheme("address-book-new"), i18n("New entry"));
        p->actionCollection()->setDefaultShortcut(newEntry, Qt::CTRL + Qt::SHIFT + Qt::Key_N);
        connect(newEntry, SIGNAL(triggered()), signalMapperNewElement, SLOT(map()));
        signalMapperNewElement->setMapping(newEntry, smEntry);
        QAction *newComment = newElementMenu->addAction(QIcon::fromTheme("address-book-new"), i18n("New comment"));
        connect(newComment, SIGNAL(triggered()), signalMapperNewElement, SLOT(map()));
        signalMapperNewElement->setMapping(newComment, smComment);
        QAction *newMacro = newElementMenu->addAction(QIcon::fromTheme("address-book-new"), i18n("New macro"));
        connect(newMacro, SIGNAL(triggered()), signalMapperNewElement, SLOT(map()));
        signalMapperNewElement->setMapping(newMacro, smMacro);
        QAction *newPreamble = newElementMenu->addAction(QIcon::fromTheme("address-book-new"), i18n("New preamble"));
        connect(newPreamble, SIGNAL(triggered()), signalMapperNewElement, SLOT(map()));
        signalMapperNewElement->setMapping(newPreamble, smPreamble);
        connect(signalMapperNewElement, SIGNAL(mapped(int)), p, SLOT(newElementTriggered(int)));

        /// Action to edit an element
        elementEditAction = new QAction(QIcon::fromTheme("document-edit"), i18n("Edit Element"), p);
        p->actionCollection()->addAction(QStringLiteral("element_edit"), elementEditAction);
        p->actionCollection()->setDefaultShortcut(elementEditAction, Qt::CTRL + Qt::Key_E);
        connect(elementEditAction, SIGNAL(triggered()), partWidget->fileView(), SLOT(editCurrentElement()));

        /// Action to view the document associated to the current element
        elementViewDocumentAction = new QAction(QIcon::fromTheme("application-pdf"), i18n("View Document"), p);
        p->actionCollection()->addAction(QStringLiteral("element_viewdocument"), elementViewDocumentAction);
        p->actionCollection()->setDefaultShortcut(elementViewDocumentAction, Qt::CTRL + Qt::Key_D);
        connect(elementViewDocumentAction, SIGNAL(triggered()), p, SLOT(elementViewDocument()));

        /// Action to find a PDF matching the current element
        elementFindPDFAction = new QAction(QIcon::fromTheme("application-pdf"), i18n("Find PDF..."), p);
        p->actionCollection()->addAction(QStringLiteral("element_findpdf"), elementFindPDFAction);
        connect(elementFindPDFAction, SIGNAL(triggered()), p, SLOT(elementFindPDF()));

        /// Action to reformat the selected elements' ids
        entryApplyDefaultFormatString = new QAction(QIcon::fromTheme("favorites"), i18n("Format entry ids"), p);
        p->actionCollection()->addAction(QStringLiteral("entry_applydefaultformatstring"), entryApplyDefaultFormatString);
        connect(entryApplyDefaultFormatString, SIGNAL(triggered()), p, SLOT(applyDefaultFormatString()));

        /// Clipboard object, required for various copy&paste operations
        Clipboard *clipboard = new Clipboard(partWidget->fileView());

        /// Actions to cut and copy selected elements as BibTeX code
        editCutAction = p->actionCollection()->addAction(KStandardAction::Cut, clipboard, SLOT(cut()));
        editCopyAction = p->actionCollection()->addAction(KStandardAction::Copy, clipboard, SLOT(copy()));

        /// Action to copy references, e.g. '\cite{fordfulkerson1959}'
        editCopyReferencesAction = new QAction(QIcon::fromTheme("edit-copy"), i18n("Copy References"), p);
        p->actionCollection()->setDefaultShortcut(editCopyReferencesAction, Qt::CTRL + Qt::SHIFT + Qt::Key_C);
        p->actionCollection()->addAction(QStringLiteral("edit_copy_references"), editCopyReferencesAction);
        connect(editCopyReferencesAction, SIGNAL(triggered()), clipboard, SLOT(copyReferences()));

        /// Action to paste BibTeX code
        editPasteAction = p->actionCollection()->addAction(KStandardAction::Paste, clipboard, SLOT(paste()));

        /// Action to delete selected rows/elements
        editDeleteAction = new QAction(QIcon::fromTheme("edit-table-delete-row"), i18n("Delete"), p);
        p->actionCollection()->setDefaultShortcut(editDeleteAction, Qt::Key_Delete);
        p->actionCollection()->addAction(QStringLiteral("edit_delete"), editDeleteAction);
        connect(editDeleteAction, SIGNAL(triggered()), partWidget->fileView(), SLOT(selectionDelete()));

        /// Build context menu for central BibTeX file view
        partWidget->fileView()->setContextMenuPolicy(Qt::ActionsContextMenu); ///< context menu is based on actions
        partWidget->fileView()->addAction(elementEditAction);
        partWidget->fileView()->addAction(elementViewDocumentAction);
        QAction *separator = new QAction(p);
        separator->setSeparator(true);
        partWidget->fileView()->addAction(separator);
        partWidget->fileView()->addAction(editCutAction);
        partWidget->fileView()->addAction(editCopyAction);
        partWidget->fileView()->addAction(editCopyReferencesAction);
        partWidget->fileView()->addAction(editPasteAction);
        partWidget->fileView()->addAction(editDeleteAction);
        separator = new QAction(p);
        separator->setSeparator(true);
        partWidget->fileView()->addAction(separator);
        partWidget->fileView()->addAction(elementFindPDFAction);
        partWidget->fileView()->addAction(entryApplyDefaultFormatString);
        colorLabelContextMenu = new ColorLabelContextMenu(partWidget->fileView());
        colorLabelContextMenuAction = p->actionCollection()->addAction(QStringLiteral("entry_colorlabel"), colorLabelContextMenu->menuAction());

        findDuplicatesUI = new FindDuplicatesUI(p, partWidget->fileView());
        lyx = new LyX(p, partWidget->fileView());

        connect(partWidget->fileView(), &FileView::selectedElementsChanged, p, &KBibTeXPart::updateActions);
        connect(partWidget->fileView(), &FileView::currentElementChanged, p, &KBibTeXPart::updateActions);
    }

    FileImporter *fileImporterFactory(const QUrl &url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "pdf") {
            return new FileImporterPDF();
        } else if (ending == "ris") {
            return new FileImporterRIS();
        } else if (BibUtils::available() && ending == "isi") {
            FileImporterBibUtils *fileImporterBibUtils = new FileImporterBibUtils();
            fileImporterBibUtils->setFormat(BibUtils::ISI);
            return fileImporterBibUtils;
        } else {
            return new FileImporterBibTeX(false);
        }
    }

    FileExporter *fileExporterFactory(const QUrl &url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "html") {
            return new FileExporterXSLT();
        } else if (ending == "xml") {
            return new FileExporterXML();
        } else if (ending == "ris") {
            return new FileExporterRIS();
        } else if (ending == "pdf") {
            return new FileExporterPDF();
        } else if (ending == "ps") {
            return new FileExporterPS();
        } else if (BibUtils::available() && ending == "isi") {
            FileExporterBibUtils *fileExporterBibUtils = new FileExporterBibUtils();
            fileExporterBibUtils->setFormat(BibUtils::ISI);
            return fileExporterBibUtils;
        } else if (ending == "rtf") {
            return new FileExporterRTF();
        } else if (ending == "html" || ending == "htm") {
            return new FileExporterBibTeX2HTML();
        } else if (ending == "bbl") {
            return new FileExporterBibTeXOutput(FileExporterBibTeXOutput::BibTeXBlockList);
        } else {
            return new FileExporterBibTeX();
        }
    }

    QString findUnusedId() {
        int i = 1;
        while (true) {
            QString result = i18n("New%1", i);
            if (!bibTeXFile->containsKey(result))
                return result;
            ++i;
        }
        return QString();
    }

    void initializeNew() {
        bibTeXFile = new File();
        model = new FileModel();
        model->setBibliographyFile(bibTeXFile);

        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), SIGNAL(filterChanged(SortFilterFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterFileModel::FilterQuery)));
    }

    bool openFile(const QUrl &url, const QString &localFilePath) {
        p->setObjectName("KBibTeXPart::KBibTeXPart for " + url.url(QUrl::PreferLocalFile));


        qApp->setOverrideCursor(Qt::WaitCursor);

        if (bibTeXFile != NULL) {
            const QUrl oldUrl = bibTeXFile->property(File::Url, QUrl()).toUrl();
            if (oldUrl.isValid() && oldUrl.isLocalFile()) {
                const QString path = oldUrl.toString();
                if (!path.isEmpty())
                    fileSystemWatcher.removePath(path);
                else
                    qCWarning(LOG_KBIBTEX_PARTS) << "No filename to stop watching";
            } else
                qCWarning(LOG_KBIBTEX_PARTS) << "Not removing" << oldUrl.toString() << "from fileSystemWatcher";
            delete bibTeXFile;
        }

        QFile inputfile(localFilePath);
        if (!inputfile.open(QIODevice::ReadOnly)) {
            qCWarning(LOG_KBIBTEX_PARTS) << "Opening file failed, creating new one instead:" << localFilePath;
            qApp->restoreOverrideCursor();
            /// Opening file failed, creating new one instead
            initializeNew();
            return false;
        }

        FileImporter *importer = fileImporterFactory(url);
        importer->showImportDialog(p->widget());
        bibTeXFile = importer->load(&inputfile);
        inputfile.close();
        delete importer;

        if (bibTeXFile == NULL) {
            qCWarning(LOG_KBIBTEX_PARTS) << "Opening file failed, creating new one instead:" << url.url(QUrl::PreferLocalFile);
            qApp->restoreOverrideCursor();
            /// Opening file failed, creating new one instead
            initializeNew();
            return false;
        }

        bibTeXFile->setProperty(File::Url, QUrl(url));

        model->setBibliographyFile(bibTeXFile);
        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), SIGNAL(filterChanged(SortFilterFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterFileModel::FilterQuery)));

        if (url.isLocalFile())
            fileSystemWatcher.addPath(url.url(QUrl::PreferLocalFile));

        qApp->restoreOverrideCursor();

        return true;
    }

    void makeBackup(const QUrl &url) const {
        /// Fetch settings from configuration
        KConfigGroup configGroup(config, Preferences::groupGeneral);
        const Preferences::BackupScope backupScope = (Preferences::BackupScope)configGroup.readEntry(Preferences::keyBackupScope, (int)Preferences::defaultBackupScope);
        const int numberOfBackups = configGroup.readEntry(Preferences::keyNumberOfBackups, Preferences::defaultNumberOfBackups);

        /// Stop right here if no backup is requested
        if (backupScope == Preferences::NoBackup)
            return;

        /// For non-local files, proceed only if backups to remote storage is allowed
        if (backupScope != Preferences::BothLocalAndRemote && !url.isLocalFile())
            return;

        /// Do not make backup copies if destination file does not exist yet
        KIO::StatJob *statJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0 /** not details necessary, just need to know if file exists */, KIO::HideProgressInfo);
        KJobWidgets::setWindow(statJob, p->widget());
        statJob->exec();
        if (statJob->error() == KIO::ERR_DOES_NOT_EXIST)
            return;
        else if (statJob->error() != KIO::Job::NoError) {
            /// Something else went wrong, quit with error
            qCWarning(LOG_KBIBTEX_PARTS) << "Probing" << url.toDisplayString() << "failed:" << statJob->errorString();
            return;
        }

        bool copySucceeded = true;
        /// Copy e.g. test.bib~ to test.bib~2, test.bib to test.bib~ etc.
        for (int level = numberOfBackups; copySucceeded && level >= 1; --level) {
            QUrl newerBackupUrl = url;
            constructBackupUrl(level - 1, newerBackupUrl);
            QUrl olderBackupUrl = url;
            constructBackupUrl(level, olderBackupUrl);

            statJob = KIO::stat(newerBackupUrl, KIO::StatJob::DestinationSide, 0 /** not details necessary, just need to know if file exists */, KIO::HideProgressInfo);
            KJobWidgets::setWindow(statJob, p->widget());
            if (statJob->exec() && statJob->error() == KIO::Job::NoError) {
                KIO::CopyJob *moveJob = KIO::move(newerBackupUrl, olderBackupUrl, KIO::HideProgressInfo | KIO::Overwrite);
                KJobWidgets::setWindow(moveJob, p->widget());
                copySucceeded = moveJob->exec();
            }
        }

        if (!copySucceeded)
            KMessageBox::error(p->widget(), i18n("Could not create backup copies of document '%1'.", url.url(QUrl::PreferLocalFile)), i18n("Backup copies"));
    }

    QUrl getSaveFilename(bool mustBeImportable = true) {
        QString startDir = p->url().isValid() ? p->url().path() : QString();
        QString supportedMimeTypes = QStringLiteral("text/x-bibtex text/x-bibtex-compiled application/xml text/x-research-info-systems");
        if (BibUtils::available())
            supportedMimeTypes += QStringLiteral(" application/x-isi-export-format application/x-endnote-refer");
        if (!mustBeImportable && !QStandardPaths::findExecutable(QStringLiteral("pdflatex")).isEmpty())
            supportedMimeTypes += QStringLiteral(" application/pdf");
        if (!mustBeImportable && !QStandardPaths::findExecutable(QStringLiteral("dvips")).isEmpty())
            supportedMimeTypes += QStringLiteral(" application/postscript");
        if (!mustBeImportable)
            supportedMimeTypes += QStringLiteral(" text/html");
        if (!mustBeImportable && !QStandardPaths::findExecutable(QStringLiteral("latex2rtf")).isEmpty())
            supportedMimeTypes += QStringLiteral(" application/rtf");

        QPointer<QFileDialog> saveDlg = new QFileDialog(p->widget(), i18n("Save file") /* TODO better text */, startDir, supportedMimeTypes);
        /// Setting list of mime types for the second time,
        /// essentially calling this function only to set the "default mime type" parameter
        saveDlg->setMimeTypeFilters(supportedMimeTypes.split(QLatin1Char(' '), QString::SkipEmptyParts));
        /// Setting the dialog into "Saving" mode make the "add extension" checkbox available
        saveDlg->setAcceptMode(QFileDialog::AcceptSave);
        saveDlg->setDefaultSuffix(QStringLiteral("bib"));
        saveDlg->setFileMode(QFileDialog::AnyFile);
        if (saveDlg->exec() != QDialog::Accepted)
            /// User cancelled saving operation, return invalid filename/URL
            return QUrl();
        const QList<QUrl> selectedUrls = saveDlg->selectedUrls();
        delete saveDlg;
        return selectedUrls.isEmpty() ? QUrl() : selectedUrls.first();
    }

    bool saveFile(const QUrl &url) {
        Q_ASSERT_X(!url.isEmpty(), "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const QUrl &url)", "url is not allowed to be empty");

        /// configure and open temporary file
        static const QRegExp suffixRegExp("\\.[^.]{1,4}$");
        const QString suffix = suffixRegExp.indexIn(url.url(QUrl::PreferLocalFile)) >= 0 ? suffixRegExp.cap(0) : QString();
        QTemporaryFile temporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QString(QStringLiteral("kbibtex_savefile_XXXXXX")).append(suffix));
        temporaryFile.setAutoRemove(true);
        if (!temporaryFile.open())
            return false;

        /// export bibliography data into temporary file
        SortFilterFileModel *model = qobject_cast<SortFilterFileModel *>(partWidget->fileView()->model());
        Q_ASSERT_X(model != NULL, "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const QUrl &url)", "SortFilterFileModel *model from editor->model() is invalid");
        FileExporter *exporter = fileExporterFactory(url);

        if (isSaveAsOperation) {
            /// only show export dialog at SaveAs or SaveCopyAs operations
            FileExporterToolchain *fet = NULL;

            if (typeid(*exporter) == typeid(FileExporterBibTeX)) {
                QPointer<QDialog> dlg = new QDialog(p->widget());
                dlg->setWindowTitle(i18n("BibTeX File Settings"));
                QBoxLayout *layout = new QVBoxLayout(dlg);
                FileSettingsWidget *settingsWidget = new FileSettingsWidget(dlg);
                layout->addWidget(settingsWidget);
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset | QDialogButtonBox::Save | QDialogButtonBox::Ok, Qt::Horizontal, dlg);
                layout->addWidget(buttonBox);
                connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, settingsWidget, &FileSettingsWidget::resetToDefaults);
                connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, settingsWidget, &FileSettingsWidget::resetToLoadedProperties);
                connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, settingsWidget, &FileSettingsWidget::applyProperties);
                connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);

                settingsWidget->loadProperties(bibTeXFile);

                if (dlg->exec() == QDialog::Accepted)
                    settingsWidget->saveProperties(bibTeXFile);
                delete dlg;
            } else if ((fet = qobject_cast<FileExporterToolchain *>(exporter)) != NULL) {
                QPointer<QDialog> dlg = new QDialog(p->widget());
                dlg->setWindowTitle(i18n("PDF/PostScript File Settings"));
                QBoxLayout *layout = new QVBoxLayout(dlg);
                SettingsFileExporterPDFPSWidget *settingsWidget = new SettingsFileExporterPDFPSWidget(dlg);
                layout->addWidget(settingsWidget);
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset | QDialogButtonBox::Save | QDialogButtonBox::Ok, Qt::Horizontal, dlg);
                layout->addWidget(buttonBox);
                connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, settingsWidget, &SettingsFileExporterPDFPSWidget::resetToDefaults);
                connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, settingsWidget, &SettingsFileExporterPDFPSWidget::loadState);
                connect(buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, settingsWidget, &SettingsFileExporterPDFPSWidget::saveState);
                connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);

                if (dlg->exec() == QDialog::Accepted)
                    settingsWidget->saveState();
                fet->reloadConfig();
                delete dlg;
            }
        }

        qApp->setOverrideCursor(Qt::WaitCursor);

        QStringList errorLog;
        bool result = exporter->save(&temporaryFile, model->fileSourceModel()->bibliographyFile(), &errorLog);
        delete exporter;

        if (!result) {
            qApp->restoreOverrideCursor();
            KMessageBox::errorList(p->widget(), i18n("Saving the bibliography to file '%1' failed.\n\nThe following output was generated by the export filter:", url.url(QUrl::PreferLocalFile)), errorLog, i18n("Saving bibliography failed"));
            return false;
        }

        /// close/flush temporary file
        temporaryFile.close();
        /// make backup before overwriting target destination
        makeBackup(url);
        /// upload temporary file to target destination
        QUrl realUrl = url;
        if (url.isLocalFile()) {
            /// take precautions for local files
            QFileInfo fileInfo(url.url(QUrl::PreferLocalFile));
            if (fileInfo.isSymLink()) {
                /// do not overwrite symbolic link,
                /// but linked file instead
                realUrl = QUrl::fromLocalFile(fileInfo.symLinkTarget());
            }
        }

        KIO::CopyJob *copyJob = KIO::copy(QUrl::fromLocalFile(temporaryFile.fileName()), realUrl, KIO::HideProgressInfo | KIO::Overwrite);
        KJobWidgets::setWindow(copyJob, p->widget());
        result &= copyJob->exec() && copyJob->error() == KIO::Job::NoError;

        qApp->restoreOverrideCursor();
        if (!result)
            KMessageBox::error(p->widget(), i18n("Saving the bibliography to file '%1' failed.", url.url(QUrl::PreferLocalFile)), i18n("Saving bibliography failed"));

        return result;
    }

    bool checkOverwrite(const QUrl &url, QWidget *parent) {
        if (!url.isLocalFile())
            return true;

        QFileInfo info(url.path());
        if (!info.exists())
            return true;

        return KMessageBox::Cancel != KMessageBox::warningContinueCancel(parent, i18n("A file named '%1' already exists. Are you sure you want to overwrite it?", info.fileName()), i18n("Overwrite File?"), KStandardGuiItem::overwrite(), KStandardGuiItem::cancel(), QString(), KMessageBox::Notify | KMessageBox::Dangerous);
    }

    /**
     * Builds or resets the menu with local and remote
     * references (URLs, files) of an entry.
     *
     * @return Number of known references
     */
    int updateViewDocumentMenu() {
        viewDocumentMenu->clear();
        int result = 0; ///< Initially, no references are known

        /// Clean signal mapper of old mappings
        /// as stored in QSet signalMapperViewDocumentSenders
        /// and identified by their QAction*'s
        QSet<QObject *>::Iterator it = signalMapperViewDocumentSenders.begin();
        while (it != signalMapperViewDocumentSenders.end()) {
            signalMapperViewDocument->removeMappings(*it);
            it = signalMapperViewDocumentSenders.erase(it);
        }

        /// Retrieve Entry object of currently selected line
        /// in main list view
        QSharedPointer<const Entry> entry = partWidget->fileView()->currentElement().dynamicCast<const Entry>();
        /// Test and continue if there was an Entry to retrieve
        if (!entry.isNull()) {
            /// Get list of URLs associated with this entry
            QList<QUrl> urlList = FileInfo::entryUrls(entry.data(), partWidget->fileView()->fileModel()->bibliographyFile()->property(File::Url).toUrl(), FileInfo::TestExistenceYes);
            if (!urlList.isEmpty()) {
                /// Memorize first action, necessary to set menu title
                QAction *firstAction = NULL;
                /// First iteration: local references only
                for (QList<QUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                    /// First iteration: local references only
                    if (!(*it).isLocalFile()) continue; ///< skip remote URLs

                    /// Build a nice menu item (label, icon, ...)
                    QFileInfo fi((*it).url(QUrl::PreferLocalFile));
                    const QString label = QString("%1 [%2]").arg(fi.fileName()).arg(fi.absolutePath());
                    QMimeDatabase db;
                    QAction *action = new QAction(QIcon::fromTheme(db.mimeTypeForUrl(*it).iconName()), label, p);
                    action->setData((*it).url(QUrl::PreferLocalFile));
                    action->setToolTip((*it).url(QUrl::PreferLocalFile));
                    /// Register action at signal handler to open URL when triggered
                    connect(action, SIGNAL(triggered()), signalMapperViewDocument, SLOT(map()));
                    signalMapperViewDocument->setMapping(action, action);
                    signalMapperViewDocumentSenders.insert(action);
                    viewDocumentMenu->addAction(action);
                    /// Memorize first action
                    if (firstAction == NULL) firstAction = action;
                }
                if (firstAction != NULL) {
                    /// If there is 'first action', then there must be
                    /// local URLs (i.e. local files) and firstAction
                    /// is the first one where a title can be set above
                    viewDocumentMenu->insertSection(firstAction, i18n("Local Files"));
                }

                firstAction = NULL; /// Now the first remote action is to be memorized
                /// Second iteration: remote references only
                for (QList<QUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                    if ((*it).isLocalFile()) continue; ///< skip local files

                    /// Build a nice menu item (label, icon, ...)
                    const QString prettyUrl = (*it).url(QUrl::PreferLocalFile);
                    QMimeDatabase db;
                    QAction *action = new QAction(QIcon::fromTheme(db.mimeTypeForUrl(*it).iconName()), prettyUrl, p);
                    action->setData(prettyUrl);
                    action->setToolTip(prettyUrl);
                    /// Register action at signal handler to open URL when triggered
                    connect(action, SIGNAL(triggered()), signalMapperViewDocument, SLOT(map()));
                    signalMapperViewDocument->setMapping(action, action);
                    signalMapperViewDocumentSenders.insert(action);
                    viewDocumentMenu->addAction(action);
                    /// Memorize first action
                    if (firstAction == NULL) firstAction = action;
                }
                if (firstAction != NULL) {
                    /// If there is 'first action', then there must be
                    /// some remote URLs and firstAction is the first
                    /// one where a title can be set above
                    viewDocumentMenu->insertSection(firstAction, i18n("Remote Files"));
                }

                result = urlList.count();
            }
        }

        return result;
    }

    void readConfiguration() {
        /// Fetch settings from configuration
        KConfigGroup configGroup(config, Preferences::groupUserInterface);
        const Preferences::ElementDoubleClickAction doubleClickAction = (Preferences::ElementDoubleClickAction)configGroup.readEntry(Preferences::keyElementDoubleClickAction, (int)Preferences::defaultElementDoubleClickAction);

        disconnect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), partWidget->fileView(), SLOT(editElement(QSharedPointer<Element>)));
        disconnect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), p, SLOT(elementViewDocument()));
        switch (doubleClickAction) {
        case Preferences::ActionOpenEditor:
            connect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), partWidget->fileView(), SLOT(editElement(QSharedPointer<Element>)));
            break;
        case Preferences::ActionViewDocument:
            connect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), p, SLOT(elementViewDocument()));
            break;
        }
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, const QVariantList &args)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(parentWidget, this))
{
    Q_UNUSED(args)
    // FIXME setComponentData(KBibTeXPartFactory::componentData(), false);

    setWidget(d->partWidget);
    updateActions();

    d->initializeNew();

    setXMLFile(RCFileName);

    new BrowserExtension(this);

    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    d->readConfiguration();

    setModified(false);
}

KBibTeXPart::~KBibTeXPart()
{
    delete d->findDuplicatesUI;
    delete d;
}

void KBibTeXPart::setModified(bool modified)
{
    KParts::ReadWritePart::setModified(modified);

    d->fileSaveAction->setEnabled(modified);
}

void KBibTeXPart::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged)
        d->readConfiguration();
}

bool KBibTeXPart::saveFile()
{
    Q_ASSERT_X(isReadWrite(), "bool KBibTeXPart::saveFile()", "Trying to save although document is in read-only mode");

    if (url().isEmpty())
        return documentSaveAs();

    /// If the current file is "watchable" (i.e. a local file),
    /// memorize local filename for future reference
    const QString watchableFilename = url().isValid() && url().isLocalFile() ? url().url(QUrl::PreferLocalFile) : QString();
    /// Stop watching local file that will be written to
    if (!watchableFilename.isEmpty())
        d->fileSystemWatcher.removePath(watchableFilename);
    else
        qCWarning(LOG_KBIBTEX_PARTS) << "watchableFilename is Empty";

    const bool saveOperationSuccess = d->saveFile(QUrl::fromLocalFile(localFilePath()));

    /// Continue watching local file after write operation
    if (!watchableFilename.isEmpty())
        d->fileSystemWatcher.addPath(watchableFilename);
    else
        qCWarning(LOG_KBIBTEX_PARTS) << "watchableFilename is Empty";

    if (!saveOperationSuccess) {
        KMessageBox::error(widget(), i18n("The document could not be saved, as it was not possible to write to '%1'.\n\nCheck that you have write access to this file or that enough disk space is available.", url().url(QUrl::PreferLocalFile)));
        return false;
    }

    return true;
}

bool KBibTeXPart::documentSave()
{
    d->isSaveAsOperation = false;
    if (!isReadWrite())
        return documentSaveCopyAs();
    else if (!url().isValid())
        return documentSaveAs();
    else
        return KParts::ReadWritePart::save();
}

bool KBibTeXPart::documentSaveAs()
{
    d->isSaveAsOperation = true;
    QUrl newUrl = d->getSaveFilename();
    if (!newUrl.isValid() || !d->checkOverwrite(newUrl, widget()))
        return false;

    /// Remove old URL from file system watcher
    if (url().isValid() && url().isLocalFile()) {
        const QString path = url().url(QUrl::PreferLocalFile);
        if (!path.isEmpty())
            d->fileSystemWatcher.removePath(path);
        else
            qCWarning(LOG_KBIBTEX_PARTS) << "No filename to stop watching";
    } else
        qCWarning(LOG_KBIBTEX_PARTS) << "Not removing" << url().url(QUrl::PreferLocalFile) << "from fileSystemWatcher";

    // TODO how does SaveAs dialog know which mime types to support?
    if (KParts::ReadWritePart::saveAs(newUrl)) {
        // FIXME d->model->bibliographyFile()->setProperty(File::Url, newUrl);
        return true;
    } else
        return false;
}

bool KBibTeXPart::documentSaveCopyAs()
{
    d->isSaveAsOperation = true;
    QUrl newUrl = d->getSaveFilename(false);
    if (!newUrl.isValid() || !d->checkOverwrite(newUrl, widget()) || newUrl == url())
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(newUrl);
}

void KBibTeXPart::elementViewDocument()
{
    QUrl url;

    QList<QAction *> actionList = d->viewDocumentMenu->actions();
    /// Go through all actions (i.e. document URLs) for this element
    for (QList<QAction *>::ConstIterator it = actionList.constBegin(); it != actionList.constEnd(); ++it) {
        /// Make URL from action's data ...
        QUrl tmpUrl = QUrl((*it)->data().toString());
        /// ... but skip this action if the URL is invalid
        if (!tmpUrl.isValid()) continue;
        if (tmpUrl.isLocalFile()) {
            /// If action's URL points to local file,
            /// keep it and stop search for document
            url = tmpUrl;
            break;
        } else if (!url.isValid())
            /// First valid URL found, keep it
            /// URL is not local, so it may get overwritten by another URL
            url = tmpUrl;
    }

    /// Open selected URL
    if (url.isValid()) {
        /// Guess mime type for url to open
        QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
        const QString mimeTypeName = mimeType.name();
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(url, mimeTypeName, widget(), false, false);
    }
}

void KBibTeXPart::elementViewDocumentMenu(QObject *obj)
{
    QString text = static_cast<QAction *>(obj)->data().toString(); ///< only a QAction will be passed along

    /// Guess mime type for url to open
    QUrl url(text);
    QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
    const QString mimeTypeName = mimeType.name();
    /// Ask KDE subsystem to open url in viewer matching mime type
    KRun::runUrl(url, mimeTypeName, widget(), false, false);
}

void KBibTeXPart::elementFindPDF()
{
    QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
    if (mil.count() == 1) {
        QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(*mil.constBegin()).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            FindPDFUI::interactiveFindPDF(*entry, *d->bibTeXFile, widget());
    }
}

void KBibTeXPart::applyDefaultFormatString()
{
    QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
    foreach (const QModelIndex &index, mil) {
        QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
        if (!entry.isNull()) {
            static IdSuggestions idSuggestions;
            bool success = idSuggestions.applyDefaultFormatId(*entry.data());
            if (!success) {
                KMessageBox::information(widget(), i18n("Cannot apply default formatting for entry ids: No default format specified."), i18n("Cannot Apply Default Formatting"));
                break;
            }
        }
    }

}

bool KBibTeXPart::openFile()
{
    const bool success = d->openFile(url(), localFilePath());
    emit completed();
    return success;
}

void KBibTeXPart::newElementTriggered(int event)
{
    switch (event) {
    case smComment:
        newCommentTriggered();
        break;
    case smMacro:
        newMacroTriggered();
        break;
    case smPreamble:
        newPreambleTriggered();
        break;
    default:
        newEntryTriggered();
    }
}

void KBibTeXPart::newEntryTriggered()
{
    QSharedPointer<Entry> newEntry = QSharedPointer<Entry>(new Entry(QStringLiteral("Article"), d->findUnusedId()));
    d->model->insertRow(newEntry, d->model->rowCount());
    d->partWidget->fileView()->setSelectedElement(newEntry);
    if (d->partWidget->fileView()->editElement(newEntry))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newMacroTriggered()
{
    QSharedPointer<Macro> newMacro = QSharedPointer<Macro>(new Macro(d->findUnusedId()));
    d->model->insertRow(newMacro, d->model->rowCount());
    d->partWidget->fileView()->setSelectedElement(newMacro);
    if (d->partWidget->fileView()->editElement(newMacro))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newPreambleTriggered()
{
    QSharedPointer<Preamble> newPreamble = QSharedPointer<Preamble>(new Preamble());
    d->model->insertRow(newPreamble, d->model->rowCount());
    d->partWidget->fileView()->setSelectedElement(newPreamble);
    if (d->partWidget->fileView()->editElement(newPreamble))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newCommentTriggered()
{
    QSharedPointer<Comment> newComment = QSharedPointer<Comment>(new Comment());
    d->model->insertRow(newComment, d->model->rowCount());
    d->partWidget->fileView()->setSelectedElement(newComment);
    if (d->partWidget->fileView()->editElement(newComment))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::updateActions()
{
    bool emptySelection = d->partWidget->fileView()->selectedElements().isEmpty();
    d->elementEditAction->setEnabled(!emptySelection);
    d->editCopyAction->setEnabled(!emptySelection);
    d->editCopyReferencesAction->setEnabled(!emptySelection);
    d->editCutAction->setEnabled(!emptySelection && isReadWrite());
    d->editPasteAction->setEnabled(isReadWrite());
    d->editDeleteAction->setEnabled(!emptySelection && isReadWrite());
    d->elementFindPDFAction->setEnabled(!emptySelection && isReadWrite());
    d->entryApplyDefaultFormatString->setEnabled(!emptySelection && isReadWrite());
    d->colorLabelContextMenu->menuAction()->setEnabled(!emptySelection && isReadWrite());
    d->colorLabelContextMenuAction->setEnabled(!emptySelection && isReadWrite());

    int numDocumentsToView = d->updateViewDocumentMenu();
    /// enable menu item only if there is at least one document to view
    d->elementViewDocumentAction->setEnabled(!emptySelection && numDocumentsToView > 0);
    /// activate sub-menu only if there are at least two documents to view
    d->elementViewDocumentAction->setMenu(numDocumentsToView > 1 ? d->viewDocumentMenu : NULL);
    d->elementViewDocumentAction->setToolTip(numDocumentsToView == 1 ? d->viewDocumentMenu->actions().first()->text() : QStringLiteral(""));

    /// update list of references which can be sent to LyX
    QStringList references;
    if (d->partWidget->fileView()->selectionModel() != NULL) {
        QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(*it).row()).dynamicCast<Entry>();
            if (!entry.isNull())
                references << entry->id();
        }
    }
    d->lyx->setReferences(references);
}

void KBibTeXPart::fileExternallyChange(const QString &path)
{
    /// Should never happen: triggering this slot for non-local or invalid URLs
    if (!url().isValid() || !url().isLocalFile())
        return;
    /// Should never happen: triggering this slot for filenames not being the opened file
    if (path != url().url(QUrl::PreferLocalFile)) {
        qCWarning(LOG_KBIBTEX_PARTS) << "Got file modification warning for wrong file: " << path << "!=" << url().url(QUrl::PreferLocalFile);
        return;
    }

    /// Stop watching file while asking for user interaction
    if (!path.isEmpty())
        d->fileSystemWatcher.removePath(path);
    else
        qCWarning(LOG_KBIBTEX_PARTS) << "No filename to stop watching";

    if (KMessageBox::warningContinueCancel(widget(), i18n("The file '%1' has changed on disk.\n\nReload file or ignore changes on disk?", path), i18n("File changed externally"), KGuiItem(i18n("Reload file"), QIcon::fromTheme("edit-redo")), KGuiItem(i18n("Ignore on-disk changes"), QIcon::fromTheme("edit-undo"))) == KMessageBox::Continue) {
        d->openFile(QUrl::fromLocalFile(path), path);
        /// No explicit call to QFileSystemWatcher.addPath(...) necessary,
        /// openFile(...) has done that already
    } else {
        /// Even if the user did not request reloaded the file,
        /// still resume watching file for future external changes
        if (!path.isEmpty())
            d->fileSystemWatcher.addPath(path);
        else
            qCWarning(LOG_KBIBTEX_PARTS) << "path is Empty";
    }
}

#include "part.moc"


