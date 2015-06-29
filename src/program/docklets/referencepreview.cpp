/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                              and contributors                           *
 *                                                                         *
 *   Contributions to this file were made by                               *
 *   - Jurgen Spitzmuller <juergen@spitzmueller.org>                       *
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

#include "referencepreview.h"

#include <QFrame>
#include <QBuffer>
#ifdef HAVE_WEBENGINEWIDGETS
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
#include <QtWebKitWidgets/QtWebKitWidgets>
#else // HAVE_WEBKITWIDGETS
#include <QTextEdit>
#include <QTextDocument>
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
#include <QLayout>
#include <QApplication>
#include <QStandardPaths>
#include <QTextStream>
#include <QTemporaryFile>
#include <QPalette>
#include <QMimeType>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <QFontDatabase>

#include <KLocalizedString>
#include <KComboBox>
#include <KRun>
#include <KIO/CopyJob>
#include <KJobWidgets>
#include <KSharedConfig>
#include <KConfigGroup>

#include "fileexporterbibtex.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterris.h"
#include "fileexporterxslt.h"
#include "element.h"
#include "file.h"
#include "entry.h"
#include "fileview.h"

static const struct PreviewStyles {
    QString label, style, type;
}
previewStyles[] = {
    {i18n("Source (BibTeX)"), QLatin1String("bibtex"), QLatin1String("exporter")},
    {i18n("Source (RIS)"), QLatin1String("ris"), QLatin1String("exporter")},
    {QLatin1String("abbrv"), QLatin1String("abbrv"), QLatin1String("bibtex2html")},
    {QLatin1String("acm"), QLatin1String("acm"), QLatin1String("bibtex2html")},
    {QLatin1String("alpha"), QLatin1String("alpha"), QLatin1String("bibtex2html")},
    {QLatin1String("apalike"), QLatin1String("apalike"), QLatin1String("bibtex2html")},
    {QLatin1String("ieeetr"), QLatin1String("ieeetr"), QLatin1String("bibtex2html")},
    {QLatin1String("plain"), QLatin1String("plain"), QLatin1String("bibtex2html")},
    {QLatin1String("siam"), QLatin1String("siam"), QLatin1String("bibtex2html")},
    {QLatin1String("unsrt"), QLatin1String("unsrt"), QLatin1String("bibtex2html")},
    {i18n("Standard"), QLatin1String("standard"), QLatin1String("xml")},
    {i18n("Fancy"), QLatin1String("fancy"), QLatin1String("xml")},
    {i18n("Wikipedia Citation"), QLatin1String("wikipedia-cite"), QLatin1String("plain_xml")},
    {i18n("Abstract-only"), QLatin1String("abstractonly"), QLatin1String("xml")}
};
static const int previewStylesLen = sizeof(previewStyles) / sizeof(previewStyles[0]);

Q_DECLARE_METATYPE(PreviewStyles)

class ReferencePreview::ReferencePreviewPrivate
{
private:
    ReferencePreview *p;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyName;

    QPushButton *buttonOpen, *buttonSaveAsHTML;
    QString htmlText;
#ifdef HAVE_WEBENGINEWIDGETS
    QWebEngineView *htmlView;
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
    QWebView *htmlView;
#else // HAVE_WEBKITWIDGETS
    QTextDocument *htmlDocument;
    QTextEdit *htmlView;
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
    KComboBox *comboBox;
    QSharedPointer<const Element> element;
    const File *file;
    FileView *fileView;
    const QColor textColor;
    const int defaultFontSize;
    const QString htmlStart;
    const QString notAvailableMessage;

    ReferencePreviewPrivate(ReferencePreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("Reference Preview Docklet")),
          configKeyName(QLatin1String("Style")), file(NULL), fileView(NULL),
          textColor(QApplication::palette().text().color()),
          defaultFontSize(QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize()),
          htmlStart("<html>\n<head>\n<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n<style type=\"text/css\">\npre {\n white-space: pre-wrap;\n white-space: -moz-pre-wrap;\n white-space: -pre-wrap;\n white-space: -o-pre-wrap;\n word-wrap: break-word;\n}\n</style>\n</head>\n<body style=\"color: " + textColor.name() + "; font-size: " + QString::number(defaultFontSize) + "pt; font-family: '" + QFontDatabase::systemFont(QFontDatabase::GeneralFont).family() + "';\">"),
          notAvailableMessage(htmlStart + "<p style=\"font-style: italic;\">" + i18n("No preview available") + "</p><p style=\"font-size: 90%;\">" + i18n("Reason:") + " %1</p></body></html>") {
        QGridLayout *gridLayout = new QGridLayout(p);
        gridLayout->setMargin(0);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);

        comboBox = new KComboBox(p);
        gridLayout->addWidget(comboBox, 0, 0, 1, 3);

        QFrame *frame = new QFrame(p);
        gridLayout->addWidget(frame, 1, 0, 1, 3);
        frame->setFrameShadow(QFrame::Sunken);
        frame->setFrameShape(QFrame::StyledPanel);

        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setMargin(0);
#ifdef HAVE_WEBENGINEWIDGETS
        htmlView = new QWebEngineView(frame);
        connect(htmlView->page(), &QWebEnginePage::urlChanged, p, &ReferencePreview::linkClicked);
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
        htmlView = new QWebView(frame);
        htmlView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
        connect(htmlView->page(), &QWebPage::linkClicked, p, &ReferencePreview::linkClicked);
#else // HAVE_WEBKITWIDGETS
        htmlView = new QTextEdit(frame);
        htmlView->setReadOnly(true);
        htmlDocument = new QTextDocument(htmlView);
        htmlView->setDocument(htmlDocument);
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
        layout->addWidget(htmlView);

        buttonOpen = new QPushButton(QIcon::fromTheme("document-open"), i18n("Open"), p);
        buttonOpen->setToolTip(i18n("Open reference in web browser."));
        gridLayout->addWidget(buttonOpen, 2, 1, 1, 1);

        buttonSaveAsHTML = new QPushButton(QIcon::fromTheme("document-save"), i18n("Save as HTML"), p);
        buttonSaveAsHTML->setToolTip(i18n("Save reference as HTML fragment."));
        gridLayout->addWidget(buttonSaveAsHTML, 2, 2, 1, 1);
    }

    bool saveHTML(const QUrl &url) const {
        QTemporaryFile file;
        file.setAutoRemove(true);

        bool result = saveHTML(file);

        if (result) {
            KIO::CopyJob *copyJob = KIO::copy(QUrl::fromLocalFile(file.fileName()), url, KIO::Overwrite);
            KJobWidgets::setWindow(copyJob, p);
            result = copyJob->exec();
        }

        return result;
    }

    bool saveHTML(QTemporaryFile &file) const {
        if (file.open()) {
            QTextStream ts(&file);
            ts.setCodec("utf-8");
            ts << QString(htmlText).replace(QRegExp("<a[^>]+href=\"kbibtex:[^>]+>([^<]+)</a>"), "\\1");
            file.close();
            return true;
        }

        return false;
    }

    void loadState() {
        static bool hasBibTeX2HTML = !QStandardPaths::findExecutable(QLatin1String("bibtex2html")).isEmpty();

        int styleIndex = 0;
        KConfigGroup configGroup(config, configGroupName);
        const QString previousStyle = configGroup.readEntry(configKeyName, QString());

        comboBox->clear();
        for (int i = 0, c = 0; i < previewStylesLen; ++i) {
            if (!hasBibTeX2HTML && previewStyles[i].type.contains(QLatin1String("bibtex2html"))) continue;
            comboBox->addItem(previewStyles[i].label, QVariant::fromValue(previewStyles[i]));
            if (previousStyle == previewStyles[i].style)
                styleIndex = c;
            ++c;
        }
        comboBox->setCurrentIndex(styleIndex);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyName, comboBox->itemData(comboBox->currentIndex()).value<PreviewStyles>().style);
        config->sync();
    }
};

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWidget(parent), d(new ReferencePreviewPrivate(this))
{
    d->loadState();

    connect(d->buttonOpen, SIGNAL(clicked()), this, SLOT(openAsHTML()));
    connect(d->buttonSaveAsHTML, SIGNAL(clicked()), this, SLOT(saveAsHTML()));
    connect(d->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(renderHTML()));

    setEnabled(false);
}

ReferencePreview::~ReferencePreview()
{
    delete d;
}

void ReferencePreview::setHtml(const QString &html)
{
    d->htmlText = QString(html).remove(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
#ifdef HAVE_WEBENGINEWIDGETS
    d->htmlView->setHtml(d->htmlText, QUrl());
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
    d->htmlView->setHtml(d->htmlText, QUrl());
#else // HAVE_WEBKITWIDGETS
    d->htmlDocument->setHtml(d->htmlText);
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
    d->buttonOpen->setEnabled(true);
    d->buttonSaveAsHTML->setEnabled(true);
}

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        setHtml(d->htmlText);
    else {
        static const QString html = d->notAvailableMessage.arg(i18n("Preview disabled"));
#ifdef HAVE_WEBENGINEWIDGETS
        d->htmlView->setHtml(html, QUrl());
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
        d->htmlView->setHtml(d->htmlText, QUrl());
#else // HAVE_WEBKITWIDGETS
        d->htmlDocument->setHtml(html);
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
        d->buttonOpen->setEnabled(false);
        d->buttonSaveAsHTML->setEnabled(false);
    }
    d->htmlView->setEnabled(enabled);
    d->comboBox->setEnabled(enabled);
}

void ReferencePreview::setElement(QSharedPointer<Element> element, File *file)
{
    d->element = element;
    d->file = file;
    renderHTML();
}

void ReferencePreview::renderHTML()
{
    enum { ignore, /// do not include crossref'ed entry's values (one entry)
           add, /// feed both the current entry as well as the crossref'ed entry into the exporter (two entries)
           merge /// merge the crossref'ed entry's values into the current entry (one entry)
         } crossRefHandling = ignore;

    if (d->element.isNull()) {
        static const QString html = d->notAvailableMessage.arg(i18n("No element selected"));
#ifdef HAVE_WEBENGINEWIDGETS
        d->htmlView->setHtml(html, QUrl());
#else // HAVE_WEBENGINEWIDGETS
#ifdef HAVE_WEBKITWIDGETS
        d->htmlView->setHtml(html, QUrl());
#else // HAVE_WEBKITWIDGETS
        d->htmlDocument->setHtml(html);
#endif // HAVE_WEBKITWIDGETS
#endif // HAVE_WEBENGINEWIDGETS
        d->buttonOpen->setEnabled(false);
        d->buttonSaveAsHTML->setEnabled(false);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    FileExporter *exporter = NULL;

    const PreviewStyles previewStyle = d->comboBox->itemData(d->comboBox->currentIndex()).value<PreviewStyles>();

    if (previewStyle.type == QLatin1String("exporter")) {
        if (previewStyle.style == QLatin1String("bibtex")) {
            FileExporterBibTeX *exporterBibTeX = new FileExporterBibTeX();
            exporterBibTeX->setEncoding(QLatin1String("utf-8"));
            exporter = exporterBibTeX;
        } else if (previewStyle.style == QLatin1String("ris"))
            exporter = new FileExporterRIS();
    } else if (previewStyle.type == QLatin1String("bibtex2html")) {
        crossRefHandling = merge;
        FileExporterBibTeX2HTML *exporterHTML = new FileExporterBibTeX2HTML();
        exporterHTML->setLaTeXBibliographyStyle(previewStyle.style);
        exporter = exporterHTML;
    } else if (previewStyle.type == QLatin1String("xml") || previewStyle.type.endsWith(QLatin1String("_xml"))) {
        crossRefHandling = merge;
        FileExporterXSLT *exporterXSLT = new FileExporterXSLT();
        QString filename = previewStyle.style + ".xsl";
        exporterXSLT->setXSLTFilename(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kbibtex/") + filename));
        exporter = exporterXSLT;
    } else
        qWarning() << "Don't know how to handle output type " << previewStyle.type;

    if (exporter != NULL) {
        QBuffer buffer(this);
        buffer.open(QBuffer::WriteOnly);

        bool exporterResult = false;
        QStringList errorLog;
        QSharedPointer<const Entry> entry = d->element.dynamicCast<const Entry>();
        if (crossRefHandling == add && !entry.isNull()) {
            QString crossRef = PlainTextValue::text(entry->value(QLatin1String("crossref")));
            QSharedPointer<const Entry> crossRefEntry = d->file == NULL ? QSharedPointer<const Entry>() : d->file->containsKey(crossRef) .dynamicCast<const Entry>();
            if (!crossRefEntry.isNull()) {
                File file;
                file.append(QSharedPointer<Entry>(new Entry(*entry)));
                file.append(QSharedPointer<Entry>(new Entry(*crossRefEntry)));
                exporterResult = exporter->save(&buffer, &file, &errorLog);
            } else
                exporterResult = exporter->save(&buffer, d->element, d->file, &errorLog);
        } else if (crossRefHandling == merge && !entry.isNull()) {
            QSharedPointer<Entry> merged = QSharedPointer<Entry>(Entry::resolveCrossref(*entry, d->file));
            exporterResult = exporter->save(&buffer, merged, d->file, &errorLog);
        } else
            exporterResult = exporter->save(&buffer, d->element, d->file, &errorLog);
        buffer.close();
        delete exporter;

        buffer.open(QBuffer::ReadOnly);
        QString text = QString::fromUtf8(buffer.readAll().data());
        buffer.close();

        if (!exporterResult || text.isEmpty()) {
            /// something went wrong, no output ...
            text = d->notAvailableMessage.arg(i18n("No HTML output generated"));
            qDebug() << errorLog.join("\n");
        } else {
            /// beautify text
            text.replace(QLatin1String("``"), QLatin1String("&ldquo;"));
            text.replace(QLatin1String("''"), QLatin1String("&rdquo;"));
            static const QRegExp openingSingleQuotationRegExp(QLatin1String("(^|[> ,.;:!?])`(\\S)"));
            static const QRegExp closingSingleQuotationRegExp(QLatin1String("(\\S)'([ ,.;:!?<]|$)"));
            text.replace(openingSingleQuotationRegExp, QLatin1String("\\1&lsquo;\\2"));
            text.replace(closingSingleQuotationRegExp, QLatin1String("\\1&rsquo;\\2"));

            if (previewStyle.style == QLatin1String("wikipedia-cite"))
                text.remove(QLatin1String("\n"));

            if (text.contains(QLatin1String("{{cite FIXME"))) {
                /// Wikipedia {{cite ...}} command had problems (e.g. unknown entry type)
                text = d->notAvailableMessage.arg(i18n("This type of element is not supported by Wikipedia's <tt>{{cite}}</tt> command."));
            } else if (previewStyle.type == QLatin1String("exporter") || previewStyle.type.startsWith(QLatin1String("plain_"))) {
                /// source
                text.prepend(QLatin1String("';\">"));
                text.prepend(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
                text.prepend(QLatin1String("<pre style=\"font-family: '"));
                text.prepend(d->htmlStart);
                text.append(QLatin1String("</pre></body></html>"));
            } else if (previewStyle.type == QLatin1String("bibtex2html")) {
                /// bibtex2html

                /// remove "generated by" line from HTML code if BibTeX2HTML was used
                text.remove(QRegExp("<hr><p><em>.*</p>"));
                text.remove(QRegExp("<[/]?(font)[^>]*>"));
                QRegExp reTable("^.*<td.*</td.*<td>");
                reTable.setMinimal(true);
                text.remove(reTable);
                text.remove(QRegExp("</td>.*$"));
                QRegExp reAnchor("\\[ <a.*</a> \\]");
                reAnchor.setMinimal(true);
                text.remove(reAnchor);

                text.prepend(d->htmlStart);
                text.append("</body></html>");
            } else if (previewStyle.type == QLatin1String("xml")) {
                /// XML/XSLT
                text.prepend(d->htmlStart);
                text.append("</body></html>");
            }

            /// adopt current color scheme
            text.replace(QLatin1String("color: black;"), QString(QLatin1String("color: %1;")).arg(d->textColor.name()));
        }

        setHtml(text);

        d->saveState();
    }

    QApplication::restoreOverrideCursor();
}

void ReferencePreview::openAsHTML()
{
    QTemporaryFile file(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QLatin1String("referencePreview-openAsHTML-XXXXXX.html"));
    file.setAutoRemove(false); /// let file stay alive for browser
    d->saveHTML(file);

    /// Ask KDE subsystem to open url in viewer matching mime type
    QUrl url(file.fileName());
    KRun::runUrl(url, QLatin1String("text/html"), this, false, false);
}

void ReferencePreview::saveAsHTML()
{
    QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Save as HTML"), QUrl(), QLatin1String("text/html"));
    if (url.isValid())
        d->saveHTML(url);
}

void ReferencePreview::linkClicked(const QUrl &url)
{
    QString text = url.toString();
    if (text.startsWith(QLatin1String("kbibtex:filter:"))) {
        text = text.mid(15);
        if (d->fileView != NULL) {
            int p = text.indexOf("=");
            SortFilterFileModel::FilterQuery fq;
            fq.terms << text.mid(p + 1);
            fq.combination = SortFilterFileModel::EveryTerm;
            fq.field = text.left(p);
            fq.searchPDFfiles = false;
            d->fileView->setFilterBarFilter(fq);
        }
    }
}

void ReferencePreview::setFileView(FileView *fileView)
{
    d->fileView = fileView;
}
