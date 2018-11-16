/***************************************************************************
 *   Copyright (C) 2004-2020 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "part.h"

#include <QLabel>
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QApplication>
#include <QLayout>
#include <QKeyEvent>
#include <QMimeType>
#include <QPointer>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTimer>
#include <QStandardPaths>
#include <QFlags>

#include <KMessageBox> // FIXME deprecated
#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>
#include <KActionMenu>
#include <KSelectAction>
#include <KToggleAction>
#include <KRun>
#include <KPluginFactory>
#include <KIO/StatJob>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KJobWidgets>
#include <kio_version.h>

#include <Preferences>
#include <File>
#include <Macro>
#include <Preamble>
#include <Comment>
#include <FileInfo>
#include <FileExporterBibTeXOutput>
#include <FileImporterBibTeX>
#include <FileExporterBibTeX>
#include <FileImporterRIS>
#include <FileImporterBibUtils>
#include <FileExporterRIS>
#include <FileExporterBibUtils>
#include <FileImporterPDF>
#include <FileExporterPS>
#include <FileExporterPDF>
#include <FileExporterRTF>
#include <FileExporterBibTeX2HTML>
#include <FileExporterXML>
#include <FileExporterXSLT>
#include <models/FileModel>
#include <IdSuggestions>
#include <LyX>
#include <UrlChecker>
#include <widgets/FileSettingsWidget>
#include <widgets/FilterBar>
#include <element/FindPDFUI>
#include <file/FileView>
#include <file/FindDuplicatesUI>
#include <file/Clipboard>
#include <preferences/SettingsColorLabelWidget>
#include <preferences/SettingsFileExporterPDFPSWidget>
#include <ValueListModel>
#include "logging_part.h"

static const char RCFileName[] = "kbibtexpartui.rc";
class KBibTeXPart::KBibTeXPartPrivate
{
private:
    KBibTeXPart *p;

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
    enum FileScope { scopeAllElements, scopeSelectedElements };

    File *bibTeXFile;
    PartWidget *partWidget;
    FileModel *model;
    SortFilterFileModel *sortFilterProxyModel;
    QAction *editCutAction, *editDeleteAction, *editCopyAction, *editPasteAction, *editCopyReferencesAction, *elementEditAction, *elementViewDocumentAction, *fileSaveAction, *elementFindPDFAction, *entryApplyDefaultFormatString;
    QMenu *viewDocumentMenu;
    bool isSaveAsOperation;
    LyX *lyx;
    FindDuplicatesUI *findDuplicatesUI;
    ColorLabelContextMenu *colorLabelContextMenu;
    QAction *colorLabelContextMenuAction;
    QFileSystemWatcher fileSystemWatcher;

    KBibTeXPartPrivate(QWidget *parentWidget, KBibTeXPart *parent)
            : p(parent), bibTeXFile(nullptr), model(nullptr), sortFilterProxyModel(nullptr), viewDocumentMenu(new QMenu(i18n("View Document"), parent->widget())), isSaveAsOperation(false), fileSystemWatcher(p) {
        connect(&fileSystemWatcher, &QFileSystemWatcher::fileChanged, p, &KBibTeXPart::fileExternallyChange);

        partWidget = new PartWidget(parentWidget);
        partWidget->fileView()->setReadOnly(!p->isReadWrite());
        connect(partWidget->fileView(), &FileView::modified, p, &KBibTeXPart::setModified);

        setupActions();
    }

    ~KBibTeXPartPrivate() {
        delete bibTeXFile;
        delete model;
        delete viewDocumentMenu;
        delete findDuplicatesUI;
    }


    void setupActions()
    {
        /// "Save" action
        fileSaveAction = p->actionCollection()->addAction(KStandardAction::Save);
        connect(fileSaveAction, &QAction::triggered, p, &KBibTeXPart::documentSave);
        fileSaveAction->setEnabled(false);
        QAction *action = p->actionCollection()->addAction(KStandardAction::SaveAs);
        connect(action, &QAction::triggered, p, &KBibTeXPart::documentSaveAs);
        /// "Save copy as" action
        QAction *saveCopyAsAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save Copy As..."), p);
        p->actionCollection()->addAction(QStringLiteral("file_save_copy_as"), saveCopyAsAction);
        connect(saveCopyAsAction, &QAction::triggered, p, &KBibTeXPart::documentSaveCopyAs);
        /// "Save selection" action
        QAction *saveSelectionAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save Selection..."), p);
        p->actionCollection()->addAction(QStringLiteral("file_save_selection"), saveSelectionAction);
        connect(saveSelectionAction, &QAction::triggered, p, &KBibTeXPart::documentSaveSelection);
        /// Enable "save selection" action only if there is something selected
        connect(partWidget->fileView(), &BasicFileView::hasSelectionChanged, saveSelectionAction, &QAction::setEnabled);
        saveSelectionAction->setEnabled(false);

        /// Filter bar widget
        QAction *filterWidgetAction = new QAction(i18n("Filter"), p);
        p->actionCollection()->addAction(QStringLiteral("toolbar_filter_widget"), filterWidgetAction);
        filterWidgetAction->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
        p->actionCollection()->setDefaultShortcut(filterWidgetAction, Qt::CTRL + Qt::Key_F);
        connect(filterWidgetAction, &QAction::triggered, partWidget->filterBar(), static_cast<void(QWidget::*)()>(&QWidget::setFocus));
        partWidget->filterBar()->setPlaceholderText(i18n("Filter bibliographic entries (%1)", filterWidgetAction->shortcut().toString()));

        /// Actions for creating new elements (entries, macros, ...)
        KActionMenu *newElementAction = new KActionMenu(QIcon::fromTheme(QStringLiteral("address-book-new")), i18n("New element"), p);
        p->actionCollection()->addAction(QStringLiteral("element_new"), newElementAction);
        QMenu *newElementMenu = new QMenu(newElementAction->text(), p->widget());
        newElementAction->setMenu(newElementMenu);
        connect(newElementAction, &QAction::triggered, p, &KBibTeXPart::newEntryTriggered);

        QAction *newEntry = new QAction(QIcon::fromTheme(QStringLiteral("address-book-new")), i18n("New entry"), newElementAction);
        newElementMenu->addAction(newEntry);
        p->actionCollection()->setDefaultShortcut(newEntry, Qt::CTRL + Qt::SHIFT + Qt::Key_N);
        connect(newEntry, &QAction::triggered, p, &KBibTeXPart::newEntryTriggered);

        QAction *newComment = new QAction(QIcon::fromTheme(QStringLiteral("address-book-new")), i18n("New comment"), newElementAction);
        newElementMenu->addAction(newComment);
        connect(newComment, &QAction::triggered, p, &KBibTeXPart::newCommentTriggered);

        QAction *newMacro = new QAction(QIcon::fromTheme(QStringLiteral("address-book-new")), i18n("New macro"), newElementAction);
        newElementMenu->addAction(newMacro);
        connect(newMacro, &QAction::triggered, p, &KBibTeXPart::newMacroTriggered);

        QAction *newPreamble = new QAction(QIcon::fromTheme(QStringLiteral("address-book-new")), i18n("New preamble"), newElementAction);
        newElementMenu->addAction(newPreamble);
        connect(newPreamble, &QAction::triggered, p, &KBibTeXPart::newPreambleTriggered);

        /// Action to edit an element
        elementEditAction = new QAction(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Edit Element"), p);
        p->actionCollection()->addAction(QStringLiteral("element_edit"), elementEditAction);
        p->actionCollection()->setDefaultShortcut(elementEditAction, Qt::CTRL + Qt::Key_E);
        connect(elementEditAction, &QAction::triggered, partWidget->fileView(), &FileView::editCurrentElement);

        /// Action to view the document associated to the current element
        elementViewDocumentAction = new QAction(QIcon::fromTheme(QStringLiteral("application-pdf")), i18n("View Document"), p);
        p->actionCollection()->addAction(QStringLiteral("element_viewdocument"), elementViewDocumentAction);
        p->actionCollection()->setDefaultShortcut(elementViewDocumentAction, Qt::CTRL + Qt::Key_D);
        connect(elementViewDocumentAction, &QAction::triggered, p, &KBibTeXPart::elementViewDocument);

        /// Action to find a PDF matching the current element
        elementFindPDFAction = new QAction(QIcon::fromTheme(QStringLiteral("application-pdf")), i18n("Find PDF..."), p);
        p->actionCollection()->addAction(QStringLiteral("element_findpdf"), elementFindPDFAction);
        connect(elementFindPDFAction, &QAction::triggered, p, &KBibTeXPart::elementFindPDF);

        /// Action to reformat the selected elements' ids
        entryApplyDefaultFormatString = new QAction(QIcon::fromTheme(QStringLiteral("favorites")), i18n("Format entry ids"), p);
        p->actionCollection()->addAction(QStringLiteral("entry_applydefaultformatstring"), entryApplyDefaultFormatString);
        connect(entryApplyDefaultFormatString, &QAction::triggered, p, &KBibTeXPart::applyDefaultFormatString);

        /// Clipboard object, required for various copy&paste operations
        Clipboard *clipboard = new Clipboard(partWidget->fileView());

        /// Actions to cut and copy selected elements as BibTeX code
        editCutAction = p->actionCollection()->addAction(KStandardAction::Cut, clipboard, SLOT(cut()));
        editCopyAction = p->actionCollection()->addAction(KStandardAction::Copy, clipboard, SLOT(copy()));

        /// Action to copy references, e.g. '\\cite{fordfulkerson1959}'
        editCopyReferencesAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy References"), p);
        p->actionCollection()->setDefaultShortcut(editCopyReferencesAction, Qt::CTRL + Qt::SHIFT + Qt::Key_C);
        p->actionCollection()->addAction(QStringLiteral("edit_copy_references"), editCopyReferencesAction);
        connect(editCopyReferencesAction, &QAction::triggered, clipboard, &Clipboard::copyReferences);

        /// Action to paste BibTeX code
        editPasteAction = p->actionCollection()->addAction(KStandardAction::Paste, clipboard, SLOT(paste()));

        /// Action to delete selected rows/elements
        editDeleteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-table-delete-row")), i18n("Delete"), p);
        p->actionCollection()->setDefaultShortcut(editDeleteAction, Qt::Key_Delete);
        p->actionCollection()->addAction(QStringLiteral("edit_delete"), editDeleteAction);
        connect(editDeleteAction, &QAction::triggered, partWidget->fileView(), &FileView::selectionDelete);

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
        const auto pos = ending.lastIndexOf(QStringLiteral("."));
        ending = ending.mid(pos + 1);

        if (ending == QStringLiteral("pdf")) {
            return new FileImporterPDF(p);
        } else if (ending == QStringLiteral("ris")) {
            return new FileImporterRIS(p);
        } else if (BibUtils::available() && ending == QStringLiteral("isi")) {
            FileImporterBibUtils *fileImporterBibUtils = new FileImporterBibUtils(p);
            fileImporterBibUtils->setFormat(BibUtils::Format::ISI);
            return fileImporterBibUtils;
        } else {
            FileImporterBibTeX *fileImporterBibTeX = new FileImporterBibTeX(p);
            fileImporterBibTeX->setCommentHandling(FileImporterBibTeX::CommentHandling::Keep);
            return fileImporterBibTeX;
        }
    }

    FileExporter *fileExporterFactory(const QString &ending) {
        if (ending == QStringLiteral("html")) {
            return new FileExporterHTML(p);
        } else if (ending == QStringLiteral("xml")) {
            return new FileExporterXML(p);
        } else if (ending == QStringLiteral("ris")) {
            return new FileExporterRIS(p);
        } else if (ending == QStringLiteral("pdf")) {
            return new FileExporterPDF(p);
        } else if (ending == QStringLiteral("ps")) {
            return new FileExporterPS(p);
        } else if (BibUtils::available() && ending == QStringLiteral("isi")) {
            FileExporterBibUtils *fileExporterBibUtils = new FileExporterBibUtils(p);
            fileExporterBibUtils->setFormat(BibUtils::Format::ISI);
            return fileExporterBibUtils;
        } else if (ending == QStringLiteral("rtf")) {
            return new FileExporterRTF(p);
        } else if (ending == QStringLiteral("html") || ending == QStringLiteral("htm")) {
            return new FileExporterBibTeX2HTML(p);
        } else if (ending == QStringLiteral("bbl")) {
            return new FileExporterBibTeXOutput(FileExporterBibTeXOutput::OutputType::BibTeXBlockList, p);
        } else {
            return new FileExporterBibTeX(p);
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
    }

    void initializeNew() {
        bibTeXFile = new File();
        model = new FileModel();
        model->setBibliographyFile(bibTeXFile);

        if (sortFilterProxyModel != nullptr) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), &FilterBar::filterChanged, sortFilterProxyModel, &SortFilterFileModel::updateFilter);
    }

    bool openFile(const QUrl &url, const QString &localFilePath) {
        p->setObjectName("KBibTeXPart::KBibTeXPart for " + url.toDisplayString() + " aka " + localFilePath);

        qApp->setOverrideCursor(Qt::WaitCursor);

        if (bibTeXFile != nullptr) {
            const QUrl oldUrl = bibTeXFile->property(File::Url, QUrl()).toUrl();
            if (oldUrl.isValid() && oldUrl.isLocalFile()) {
                const QString path = oldUrl.toLocalFile();
                if (!path.isEmpty())
                    fileSystemWatcher.removePath(path);
                else
                    qCWarning(LOG_KBIBTEX_PART) << "No filename to stop watching";
            }
            delete bibTeXFile;
            bibTeXFile = nullptr;
        }

        QFile inputfile(localFilePath);
        if (!inputfile.open(QIODevice::ReadOnly)) {
            qCWarning(LOG_KBIBTEX_PART) << "Opening file failed, creating new one instead:" << url.toDisplayString() << "aka" << localFilePath;
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

        if (bibTeXFile == nullptr) {
            qCWarning(LOG_KBIBTEX_PART) << "Opening file failed, creating new one instead:" << url.toDisplayString() << "aka" << localFilePath;
            qApp->restoreOverrideCursor();
            /// Opening file failed, creating new one instead
            initializeNew();
            return false;
        }

        bibTeXFile->setProperty(File::Url, QUrl(url));

        model->setBibliographyFile(bibTeXFile);
        if (sortFilterProxyModel != nullptr) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), &FilterBar::filterChanged, sortFilterProxyModel, &SortFilterFileModel::updateFilter);

        if (url.isLocalFile())
            fileSystemWatcher.addPath(url.toLocalFile());

        qApp->restoreOverrideCursor();

        return true;
    }

    void makeBackup(const QUrl &url) const {
        /// Fetch settings from configuration
        const int numberOfBackups = Preferences::instance().numberOfBackups();

        /// Stop right here if no backup is requested
        if (Preferences::instance().backupScope() == Preferences::BackupScope::None)
            return;

        /// For non-local files, proceed only if backups to remote storage is allowed
        if (Preferences::instance().backupScope() != Preferences::BackupScope::BothLocalAndRemote && !url.isLocalFile())
            return;

        /// Do not make backup copies if destination file does not exist yet
        KIO::StatJob *statJob = KIO::stat(url, KIO::StatJob::DestinationSide, 0 /** not details necessary, just need to know if file exists */, KIO::HideProgressInfo);
        KJobWidgets::setWindow(statJob, p->widget());
        statJob->exec();
        if (statJob->error() == KIO::ERR_DOES_NOT_EXIST)
            return;
        else if (statJob->error() != KIO::Job::NoError) {
            /// Something else went wrong, quit with error
            qCWarning(LOG_KBIBTEX_PART) << "Probing" << url.toDisplayString() << "failed:" << statJob->errorString();
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
                KIO::CopyJob *moveJob = nullptr; ///< guaranteed to be initialized in either branch of the following code
                /**
                 * The following 'if' block is necessary to handle the
                 * following situation: User opens, modifies, and saves
                 * file /tmp/b/bbb.bib which is actually a symlink to
                 * file /tmp/a/aaa.bib. Now a 'move' operation like the
                 * implicit 'else' section below does, would move /tmp/b/bbb.bib
                 * to become /tmp/b/bbb.bib~ still pointing to /tmp/a/aaa.bib.
                 * Then, the save operation would create a new file /tmp/b/bbb.bib
                 * without any symbolic linking to /tmp/a/aaa.bib.
                 * The following code therefore checks if /tmp/b/bbb.bib is
                 * to be copied/moved to /tmp/b/bbb.bib~ and /tmp/b/bbb.bib
                 * is a local file and /tmp/b/bbb.bib is a symbolic link to
                 * another file. Then /tmp/b/bbb.bib is resolved to the real
                 * file /tmp/a/aaa.bib which is then copied into plain file
                 * /tmp/b/bbb.bib~. The save function (outside of this function's
                 * scope) will then see that /tmp/b/bbb.bib is a symbolic link,
                 * resolve this symlink to /tmp/a/aaa.bib, and then write
                 * all changes to /tmp/a/aaa.bib keeping /tmp/b/bbb.bib a
                 * link to.
                 */
                if (level == 1 && newerBackupUrl.isLocalFile() /** for level==1, this is actually the current file*/) {
                    QFileInfo newerBackupFileInfo(newerBackupUrl.toLocalFile());
                    if (newerBackupFileInfo.isSymLink()) {
                        while (newerBackupFileInfo.isSymLink()) {
                            newerBackupUrl = QUrl::fromLocalFile(newerBackupFileInfo.symLinkTarget());
                            newerBackupFileInfo = QFileInfo(newerBackupUrl.toLocalFile());
                        }
                        moveJob = KIO::copy(newerBackupUrl, olderBackupUrl, KIO::HideProgressInfo | KIO::Overwrite);
                    }
                }
                if (moveJob == nullptr) ///< implicit 'else' section, see longer comment above
                    moveJob = KIO::move(newerBackupUrl, olderBackupUrl, KIO::HideProgressInfo | KIO::Overwrite);
                KJobWidgets::setWindow(moveJob, p->widget());
                copySucceeded = moveJob->exec();
            }
        }

        if (!copySucceeded)
            KMessageBox::error(p->widget(), i18n("Could not create backup copies of document '%1'.", url.url(QUrl::PreferLocalFile)), i18n("Backup copies"));
    }

    /**
     * Options to tell @see getSaveFileame how to prepare the file-save dialog.
     */
    enum GetSaveFilenameOption {
        gsfoImportableFiletype = 0x01, ///< List of available mime types to save as may onlz include mime types which can be loaded/imported, too
        gsfoCurrentFilenameReused = 0x02 ///< Propose the current filename as the default suggestion (file extension may get adjusted, though)
    };

    /**
     * Prepare and show a file-save dialog to the user. The dialog is prepared
     * as requested by the options, which are an OR combination of flags from
     * @see GetSaveFilenameOption
     * @param options OR combination of flags from @see GetSaveFilenameOption enum
     * @return Valid URL if querying filename in file-save dialog succeeded, invalid URL otherwise
     */
    QUrl getSaveFilename(const int options) const {
        const QString startDir = p->url().isValid() ? ((options & GetSaveFilenameOption::gsfoCurrentFilenameReused) > 0 ? p->url().path() : QFileInfo(p->url().path()).absolutePath()) : QString();
        QString supportedMimeTypes = QStringLiteral("text/x-bibtex text/x-research-info-systems");
        if (BibUtils::available())
            supportedMimeTypes += QStringLiteral(" application/x-isi-export-format application/x-endnote-refer");
        if ((options & GetSaveFilenameOption::gsfoImportableFiletype) == 0) {
            if (!QStandardPaths::findExecutable(QStringLiteral("pdflatex")).isEmpty())
                supportedMimeTypes += QStringLiteral(" application/pdf");
            if (!QStandardPaths::findExecutable(QStringLiteral("dvips")).isEmpty())
                supportedMimeTypes += QStringLiteral(" application/postscript");
            supportedMimeTypes += QStringLiteral(" text/html");
            if (!QStandardPaths::findExecutable(QStringLiteral("latex2rtf")).isEmpty())
                supportedMimeTypes += QStringLiteral(" application/rtf");
        }

        QPointer<QFileDialog> saveDlg = new QFileDialog(p->widget(), i18n("Save file") /* TODO better text */, startDir, supportedMimeTypes);
        /// Setting list of mime types for the second time,
        /// essentially calling this function only to set the "default mime type" parameter
        saveDlg->setMimeTypeFilters(supportedMimeTypes.split(QLatin1Char(' '), QString::SkipEmptyParts));
        /// Setting the dialog into "Saving" mode make the "add extension" checkbox available
        saveDlg->setAcceptMode(QFileDialog::AcceptSave);
        /// Mime type 'text/x-bibtex' is guaranteed to be pre-selected, so set default filename suffix accordingly
        saveDlg->setDefaultSuffix(QStringLiteral("bib"));
        saveDlg->setFileMode(QFileDialog::AnyFile);
        if (saveDlg->exec() != QDialog::Accepted)
            /// User cancelled saving operation, return invalid filename/URL
            return QUrl();
        const QList<QUrl> selectedUrls = saveDlg->selectedUrls();
        delete saveDlg;
        return selectedUrls.isEmpty() ? QUrl() : selectedUrls.first();
    }

    FileExporter *saveFileExporter(const QString &ending) {
        FileExporter *exporter = fileExporterFactory(ending);

        if (isSaveAsOperation) {
            /// only show export dialog at SaveAs or SaveCopyAs operations
            FileExporterToolchain *fet = nullptr;

            if (FileExporterBibTeX::isFileExporterBibTeX(*exporter)) {
                QPointer<QDialog> dlg = new QDialog(p->widget());
                dlg->setWindowTitle(i18n("BibTeX File Settings"));
                QBoxLayout *layout = new QVBoxLayout(dlg);
                FileSettingsWidget *settingsWidget = new FileSettingsWidget(dlg);
                layout->addWidget(settingsWidget);
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset | QDialogButtonBox::Ok, Qt::Horizontal, dlg);
                layout->addWidget(buttonBox);
                connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, settingsWidget, &FileSettingsWidget::resetToDefaults);
                connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, settingsWidget, &FileSettingsWidget::resetToLoadedProperties);
                connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);

                settingsWidget->loadProperties(bibTeXFile);

                if (dlg->exec() == QDialog::Accepted)
                    settingsWidget->saveProperties(bibTeXFile);
                delete dlg;
            } else if ((fet = qobject_cast<FileExporterToolchain *>(exporter)) != nullptr) {
                QPointer<QDialog> dlg = new QDialog(p->widget());
                dlg->setWindowTitle(i18n("PDF/PostScript File Settings"));
                QBoxLayout *layout = new QVBoxLayout(dlg);
                SettingsFileExporterPDFPSWidget *settingsWidget = new SettingsFileExporterPDFPSWidget(dlg);
                layout->addWidget(settingsWidget);
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset | QDialogButtonBox::Ok, Qt::Horizontal, dlg);
                layout->addWidget(buttonBox);
                connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, settingsWidget, &SettingsFileExporterPDFPSWidget::resetToDefaults);
                connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, settingsWidget, &SettingsFileExporterPDFPSWidget::loadState);
                connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);

                if (dlg->exec() == QDialog::Accepted)
                    settingsWidget->saveState();
                delete dlg;
            }
        }

        return exporter;
    }

    bool saveFile(QFile &file, const FileScope fileScope, FileExporter *exporter, QStringList *errorLog = nullptr) {
        SortFilterFileModel *model = qobject_cast<SortFilterFileModel *>(partWidget->fileView()->model());
        Q_ASSERT_X(model != nullptr, "FileExporter *KBibTeXPart::KBibTeXPartPrivate:saveFile(...)", "SortFilterFileModel *model from editor->model() is invalid");
        Q_ASSERT_X(model->fileSourceModel()->bibliographyFile() == bibTeXFile, "FileExporter *KBibTeXPart::KBibTeXPartPrivate:saveFile(...)", "SortFilterFileModel's BibTeX File does not match Part's BibTeX File");

        switch (fileScope) {
        case scopeAllElements: {
            /// Save complete file
            return exporter->save(&file, bibTeXFile, errorLog);
        } ///< no break required as there is an unconditional 'return' further above
        case scopeSelectedElements: {
            /// Save only selected elements
            const auto &list = partWidget->fileView()->selectionModel()->selectedRows();
            if (list.isEmpty()) return false; /// Empty selection? Abort here

            File fileWithSelectedElements;
            for (const QModelIndex &indexInSelection : list) {
                const QModelIndex &indexInFileModel = model->mapToSource(indexInSelection);
                const int row = indexInFileModel.row();
                const QSharedPointer<Element> &element = (*bibTeXFile)[row];
                fileWithSelectedElements << element;
            }
            return exporter->save(&file, &fileWithSelectedElements, errorLog);
        } ///< no break required as there is an unconditional 'return' further above
        }

        /// Above switch should cover all cases and each case should
        /// invoke 'return'. Thus, this code here should never be reached.
        return false;
    }

    bool saveFile(const QUrl &url, const FileScope &fileScope = scopeAllElements) {
        bool result = false;
        Q_ASSERT_X(url.isValid(), "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const QUrl &url, const FileScope&)", "url must be valid");

        /// Extract filename extension (e.g. 'bib') to determine which FileExporter to use
        static const QRegularExpression suffixRegExp(QStringLiteral("\\.([^.]{1,4})$"));
        const QRegularExpressionMatch suffixRegExpMatch = suffixRegExp.match(url.fileName());
        const QString ending = suffixRegExpMatch.hasMatch() ? suffixRegExpMatch.captured(1) : QStringLiteral("bib");
        FileExporter *exporter = saveFileExporter(ending);

        /// String list to collect error message from FileExporer
        QStringList errorLog;
        qApp->setOverrideCursor(Qt::WaitCursor);

        if (url.isLocalFile()) {
            /// Take precautions for local files
            QFileInfo fileInfo(url.toLocalFile());
            /// Do not overwrite symbolic link, but linked file instead
            QString filename = fileInfo.absoluteFilePath();
            while (fileInfo.isSymLink()) {
                filename = fileInfo.symLinkTarget();
                fileInfo = QFileInfo(filename);
            }
            if (!fileInfo.exists() || fileInfo.isWritable()) {
                /// Make backup before overwriting target destination, intentionally
                /// using the provided filename, not the resolved symlink
                makeBackup(url);

                QFile file(filename);
                if (file.open(QIODevice::WriteOnly)) {
                    result = saveFile(file, fileScope, exporter, &errorLog);
                    file.close();
                }
            }
        } else {
            /// URL points to a remote location

            /// Configure and open temporary file
            QTemporaryFile temporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QStringLiteral("kbibtex_savefile_XXXXXX") + ending);
            temporaryFile.setAutoRemove(true);
            if (temporaryFile.open()) {
                result = saveFile(temporaryFile, fileScope, exporter, &errorLog);

                /// Close/flush temporary file
                temporaryFile.close();

                if (result) {
                    /// Make backup before overwriting target destination
                    makeBackup(url);

                    KIO::CopyJob *copyJob = KIO::copy(QUrl::fromLocalFile(temporaryFile.fileName()), url, KIO::HideProgressInfo | KIO::Overwrite);
                    KJobWidgets::setWindow(copyJob, p->widget());
                    result &= copyJob->exec() && copyJob->error() == KIO::Job::NoError;
                }
            }
        }

        qApp->restoreOverrideCursor();

        delete exporter;

        if (!result) {
            QString msg = i18n("Saving the bibliography to file '%1' failed.", url.toDisplayString());
            if (errorLog.isEmpty())
                KMessageBox::error(p->widget(), msg, i18n("Saving bibliography failed"));
            else {
                msg += QLatin1String("\n\n");
                msg += i18n("The following output was generated by the export filter:");
                KMessageBox::errorList(p->widget(), msg, errorLog, i18n("Saving bibliography failed"));
            }
        } else
            bibTeXFile->setProperty(File::Url, url);
        return result;
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

        /// Retrieve Entry object of currently selected line
        /// in main list view
        QSharedPointer<const Entry> entry = partWidget->fileView()->currentElement().dynamicCast<const Entry>();
        /// Test and continue if there was an Entry to retrieve
        if (!entry.isNull()) {
            /// Get list of URLs associated with this entry
            const auto urlList = FileInfo::entryUrls(entry, bibTeXFile->property(File::Url).toUrl(), FileInfo::TestExistence::Yes);
            if (!urlList.isEmpty()) {
                /// Memorize first action, necessary to set menu title
                QAction *firstAction = nullptr;
                /// First iteration: local references only
                for (const QUrl &url : urlList) {
                    /// First iteration: local references only
                    if (!url.isLocalFile()) continue; ///< skip remote URLs

                    /// Build a nice menu item (label, icon, ...)
                    const QFileInfo fi(url.toLocalFile());
                    const QString label = QString(QStringLiteral("%1 [%2]")).arg(fi.fileName(), fi.absolutePath());
                    QAction *action = new QAction(QIcon::fromTheme(FileInfo::mimeTypeForUrl(url).iconName()), label, p);
                    action->setToolTip(fi.absoluteFilePath());
                    /// Open URL when action is triggered
                    connect(action, &QAction::triggered, p, [this, fi]() {
                        elementViewDocumentMenu(QUrl::fromLocalFile(fi.absoluteFilePath()));
                    });
                    viewDocumentMenu->addAction(action);
                    /// Memorize first action
                    if (firstAction == nullptr) firstAction = action;
                }
                if (firstAction != nullptr) {
                    /// If there is 'first action', then there must be
                    /// local URLs (i.e. local files) and firstAction
                    /// is the first one where a title can be set above
                    viewDocumentMenu->insertSection(firstAction, i18n("Local Files"));
                }

                firstAction = nullptr; /// Now the first remote action is to be memorized
                /// Second iteration: remote references only
                for (const QUrl &url : urlList) {
                    if (url.isLocalFile()) continue; ///< skip local files

                    /// Build a nice menu item (label, icon, ...)
                    const QString prettyUrl = url.toDisplayString();
                    QAction *action = new QAction(QIcon::fromTheme(FileInfo::mimeTypeForUrl(url).iconName()), prettyUrl, p);
                    action->setToolTip(prettyUrl);
                    /// Open URL when action is triggered
                    connect(action, &QAction::triggered, p, [this, url]() {
                        elementViewDocumentMenu(url);
                    });
                    viewDocumentMenu->addAction(action);
                    /// Memorize first action
                    if (firstAction == nullptr) firstAction = action;
                }
                if (firstAction != nullptr) {
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
        disconnect(partWidget->fileView(), &FileView::elementExecuted, partWidget->fileView(), &FileView::editElement);
        disconnect(partWidget->fileView(), &FileView::elementExecuted, p, &KBibTeXPart::elementViewDocument);
        switch (Preferences::instance().fileViewDoubleClickAction()) {
        case Preferences::FileViewDoubleClickAction::OpenEditor:
            connect(partWidget->fileView(), &FileView::elementExecuted, partWidget->fileView(), &FileView::editElement);
            break;
        case Preferences::FileViewDoubleClickAction::ViewDocument:
            connect(partWidget->fileView(), &FileView::elementExecuted, p, &KBibTeXPart::elementViewDocument);
            break;
        }
    }

    void elementViewDocumentMenu(const QUrl &url)
    {
        const QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
        const QString mimeTypeName = mimeType.name();
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(url, mimeTypeName, p->widget(), KRun::RunFlags());
    }

};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, const KAboutData &componentData)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(parentWidget, this))
{
    setComponentData(componentData);

    setWidget(d->partWidget);
    updateActions();

    d->initializeNew();

    setXMLFile(RCFileName);

    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    d->readConfiguration();

    setModified(false);
}

KBibTeXPart::~KBibTeXPart()
{
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

    if (!url().isValid())
        return documentSaveAs();

    /// If the current file is "watchable" (i.e. a local file),
    /// memorize local filename for future reference
    const QString watchableFilename = url().isValid() && url().isLocalFile() ? url().toLocalFile() : QString();
    /// Stop watching local file that will be written to
    if (!watchableFilename.isEmpty())
        d->fileSystemWatcher.removePath(watchableFilename);
    else
        qCWarning(LOG_KBIBTEX_PART) << "watchableFilename is Empty";

    const bool saveOperationSuccess = d->saveFile(url());

    if (!watchableFilename.isEmpty()) {
        /// Continue watching a local file after write operation, but do
        /// so only after a short delay. The delay is necessary in some
        /// situations as observed in KDE bug report 396343 where the
        /// DropBox client seemingly touched the file right after saving
        /// from within KBibTeX, triggering KBibTeX to show a 'reload'
        /// message box.
        QTimer::singleShot(500, this, [this, watchableFilename]() {
            d->fileSystemWatcher.addPath(watchableFilename);
        });
    } else
        qCWarning(LOG_KBIBTEX_PART) << "watchableFilename is Empty";

    if (!saveOperationSuccess) {
        KMessageBox::error(widget(), i18n("The document could not be saved, as it was not possible to write to '%1'.\n\nCheck that you have write access to this file or that enough disk space is available.", url().toDisplayString()));
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
    QUrl newUrl = d->getSaveFilename(KBibTeXPartPrivate::gsfoCurrentFilenameReused | KBibTeXPartPrivate::gsfoImportableFiletype);
    if (!newUrl.isValid())
        return false;

    /// Remove old URL from file system watcher
    if (url().isValid() && url().isLocalFile()) {
        const QString path = url().toLocalFile();
        if (!path.isEmpty())
            d->fileSystemWatcher.removePath(path);
        else
            qCWarning(LOG_KBIBTEX_PART) << "No filename to stop watching";
    } else
        qCWarning(LOG_KBIBTEX_PART) << "Not removing" << url().url(QUrl::PreferLocalFile) << "from fileSystemWatcher";

    // TODO how does SaveAs dialog know which mime types to support?
    if (KParts::ReadWritePart::saveAs(newUrl))
        return true;
    else
        return false;
}

bool KBibTeXPart::documentSaveCopyAs()
{
    d->isSaveAsOperation = true;
    QUrl newUrl = d->getSaveFilename(KBibTeXPartPrivate::gsfoCurrentFilenameReused);
    if (!newUrl.isValid() || newUrl == url())
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(newUrl);
}

bool KBibTeXPart::documentSaveSelection()
{
    d->isSaveAsOperation = true;
    const QUrl newUrl = d->getSaveFilename(~KBibTeXPartPrivate::gsfoCurrentFilenameReused);
    if (!newUrl.isValid() || newUrl == url())
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(newUrl, KBibTeXPartPrivate::scopeSelectedElements);
}

void KBibTeXPart::elementViewDocument()
{
    QUrl url;

    const QList<QAction *> actionList = d->viewDocumentMenu->actions();
    /// Go through all actions (i.e. document URLs) for this element
    for (const QAction *action : actionList) {
        /// Make URL from action's data ...
        const QString actionData = action->data().toString();
        if (actionData.isEmpty()) continue; ///< No URL from empty string
        const QUrl tmpUrl = QUrl::fromUserInput(actionData);
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
        KRun::runUrl(url, mimeTypeName, widget(), KRun::RunFlags());
    }
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
    FileModel *model = d->partWidget != nullptr && d->partWidget->fileView() != nullptr ? d->partWidget->fileView()->fileModel() : nullptr;
    if (model == nullptr) return;

    bool documentModified = false;
    const QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
    for (const QModelIndex &index : mil) {
        QSharedPointer<Entry> entry = model->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
        if (!entry.isNull()) {
            static IdSuggestions idSuggestions;
            bool success = idSuggestions.applyDefaultFormatId(*entry.data());
            documentModified |= success;
            if (!success) {
                KMessageBox::information(widget(), i18n("Cannot apply default formatting for entry ids: No default format specified."), i18n("Cannot Apply Default Formatting"));
                break;
            }
        }
    }

    if (documentModified)
        d->partWidget->fileView()->externalModification();
}

bool KBibTeXPart::openFile()
{
    const bool success = d->openFile(url(), localFilePath());
    emit completed();
    return success;
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
    FileModel *model = d->partWidget != nullptr && d->partWidget->fileView() != nullptr ? d->partWidget->fileView()->fileModel() : nullptr;
    if (model == nullptr) return;

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
    d->elementViewDocumentAction->setMenu(numDocumentsToView > 1 ? d->viewDocumentMenu : nullptr);
    d->elementViewDocumentAction->setToolTip(numDocumentsToView == 1 ? (*d->viewDocumentMenu->actions().constBegin())->text() : QString());

    /// update list of references which can be sent to LyX
    QStringList references;
    if (d->partWidget->fileView()->selectionModel() != nullptr) {
        const QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
        references.reserve(mil.size());
        for (const QModelIndex &index : mil) {
            const QSharedPointer<Entry> entry = model->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
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
    if (path != url().toLocalFile()) {
        qCWarning(LOG_KBIBTEX_PART) << "Got file modification warning for wrong file: " << path << "!=" << url().toLocalFile();
        return;
    }

    /// Stop watching file while asking for user interaction
    if (!path.isEmpty())
        d->fileSystemWatcher.removePath(path);
    else
        qCWarning(LOG_KBIBTEX_PART) << "No filename to stop watching";

    if (KMessageBox::warningContinueCancel(widget(), i18n("The file '%1' has changed on disk.\n\nReload file or ignore changes on disk?", path), i18n("File changed externally"), KGuiItem(i18n("Reload file"), QIcon::fromTheme(QStringLiteral("edit-redo"))), KGuiItem(i18n("Ignore on-disk changes"), QIcon::fromTheme(QStringLiteral("edit-undo")))) == KMessageBox::Continue) {
        d->openFile(QUrl::fromLocalFile(path), path);
        /// No explicit call to QFileSystemWatcher.addPath(...) necessary,
        /// openFile(...) has done that already
    } else {
        /// Even if the user did not request reloaded the file,
        /// still resume watching file for future external changes
        if (!path.isEmpty())
            d->fileSystemWatcher.addPath(path);
        else
            qCWarning(LOG_KBIBTEX_PART) << "path is Empty";
    }
}
