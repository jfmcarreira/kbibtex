/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QWebPage>
#include <QWebFrame>
#include <QWebElement>
#include <QFile>

#include <KLocale>
#include <KDebug>
#include <kio/job.h>

#include <fileimporterbibtex.h>
#include "websearchspringerlink.h"

class WebSearchSpringerLink::WebSearchSpringerLinkPrivate
{
private:
    WebSearchSpringerLink *p;

public:
    const QString springerLinkBaseUrl;
    const QString springerLinkQueryUrl;
    KUrl springerLinkSearchUrl;
    const int numPages;
    QList<QWebPage*> pages;
    int numExpectedResults;
    int runningJobs;
    int currentSearchPosition;
    QStringList articleUrls;

    WebSearchSpringerLinkPrivate(WebSearchSpringerLink *parent)
            : p(parent), springerLinkBaseUrl(QLatin1String("http://www.springerlink.com")), springerLinkQueryUrl(QLatin1String("http://www.springerlink.com/content/")), numPages(8) {
        for (int i = 0; i < numPages; ++i) {
            QWebPage *page = new QWebPage(parent);
            page->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
            page->settings()->setAttribute(QWebSettings::JavaEnabled, false);
            page->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
            page->settings()->setAttribute(QWebSettings::AutoLoadImages, false);
            page->settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
            pages << page;
        }
    }

    ~WebSearchSpringerLinkPrivate() {
        while (!pages.isEmpty()) {
            QWebPage *page = pages.first();
            pages.removeFirst();
            delete page;
        }
    }

    KUrl& buildQueryUrl(KUrl& url, const QMap<QString, QString> &query) {
        QString queryString = query[queryKeyFreeText] + ' ' + query[queryKeyTitle] + ' ' + query[queryKeyYear];

        QStringList authors = p->splitRespectingQuotationMarks(query[queryKeyAuthor]);
        foreach(QString author, authors) {
            queryString += QString(QLatin1String("(au:(%1) OR ed:(%1))")).arg(author);
        }

        queryString = queryString.trimmed();

        url.addQueryItem(QLatin1String("k"), queryString);

        return url;
    }

    void sanitizeBibTeXCode(QString &code) {
        /// DOI is "hidden" in a "note" field, rename field to "DOI"
        code = code.replace(QLatin1String("note = {10."), QLatin1String("doi = {10."));
    }
};


WebSearchSpringerLink::WebSearchSpringerLink(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchSpringerLink::WebSearchSpringerLinkPrivate(this))
{
    // nothing
}

void WebSearchSpringerLink::startSearch()
{
    m_hasBeenCanceled = false;
    d->runningJobs = 0;
    d->currentSearchPosition = 0;
    d->articleUrls.clear();
    // TODO
    emit stoppedSearch(resultNoError);
}

void WebSearchSpringerLink::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->runningJobs = 0;
    d->currentSearchPosition = 0;
    d->articleUrls.clear();

    d->numExpectedResults = numResults;

    ++d->runningJobs;
    d->springerLinkSearchUrl = KUrl(d->springerLinkQueryUrl);
    d->springerLinkSearchUrl = d->buildQueryUrl(d->springerLinkSearchUrl, query);
    d->pages[0]->mainFrame()->load(d->springerLinkSearchUrl);
    connect(d->pages[0], SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));
}

QString WebSearchSpringerLink::label() const
{
    return i18n("SpringerLink");
}

QString WebSearchSpringerLink::favIconUrl() const
{
    return QLatin1String("http://www.springerlink.com/images/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchSpringerLink::customWidget(QWidget *)
{
    return NULL;
}

KUrl WebSearchSpringerLink::homepage() const
{
    return KUrl("http://www.springerlink.com/");
}

void WebSearchSpringerLink::cancel()
{
    WebSearchAbstract::cancel();
    foreach(QWebPage *page, d->pages) {
        page->triggerAction(QWebPage::Stop);
    }
}

void WebSearchSpringerLink::doneFetchingResultPage(bool ok)
{
    --d->runningJobs;
    Q_ASSERT(d->runningJobs == 0);

    QWebPage *page = d->pages[0];
    disconnect(page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));

    if (handleErrors(ok)) {
        QWebElementCollection allArticleLinks = page->mainFrame()->findAllElements(QLatin1String("ul[id=\"PrimaryManifest\"] > li > p[class=\"title\"] > a"));
        foreach(QWebElement articleLink, allArticleLinks) {
            QString url(d->springerLinkBaseUrl + articleLink.attribute(QLatin1String("href")) + QLatin1String("export-citation/"));
            d->articleUrls << url;
            if (d->articleUrls.count() >= d->numExpectedResults)
                break;
        }

        d->currentSearchPosition += 10;
        if (d->numExpectedResults > d->currentSearchPosition) {
            KUrl url(d->springerLinkSearchUrl);
            url.addQueryItem(QLatin1String("o"), QString::number(d->currentSearchPosition));
            page->mainFrame()->load(url);
            connect(page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingResultPage(bool)));
            ++d->runningJobs;
        } else {
            for (int i = 0; i < d->numPages; ++i) {
                if (d->articleUrls.isEmpty()) break;

                QString url = d->articleUrls.first();
                d->articleUrls.removeFirst();
                d->pages[i]->mainFrame()->load(url);
                connect(d->pages[i], SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingArticlePage(bool)));
                ++d->runningJobs;
            }
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else {
        kWarning() << "handleError returned false";
        cancel();
    }
}


void WebSearchSpringerLink::doneFetchingArticlePage(bool ok)
{
    --d->runningJobs;

    QWebPage *page = static_cast<QWebPage*>(sender());
    disconnect(page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingArticlePage(bool)));

    if (handleErrors(ok)) {
        QWebElementCollection allInputs = page->mainFrame()->findAllElements(QLatin1String("form[name=\"aspnetForm\"] input"));
        if (allInputs.count() < 1) kWarning() << "No input tags found";

        QString queryString = QLatin1String("ctl00%24ContentPrimary%24ctl00%24ctl00%24ExportCitationButton=Export+Citation&ctl00%24ContentPrimary%24ctl00%24ctl00%24CitationManagerDropDownList=BibTex&ctl00%24ContentPrimary%24ctl00%24ctl00%24Export=AbstractRadioButton");
        foreach(QWebElement input, allInputs) {
            if (input.attribute(QLatin1String("type")) == QLatin1String("submit")) {
                /// ignore submit buttons, will be set automatically
            } else if (input.attribute(QLatin1String("type")) == QLatin1String("radio")) {
                /// ignore radio buttons, will be set automatically
            } else if (input.attribute(QLatin1String("name")).contains(QLatin1String("SearchControl$Advanced"))) {
                /// ignore submit buttons, will be set automatically
            } else
                queryString += '&' + encodeURL(input.attribute(QLatin1String("name"))) + '=' + encodeURL(input.attribute(QLatin1String("value")));
        }

        KUrl url(page->mainFrame()->url());
        KIO::TransferJob *job = KIO::storedHttpPost(queryString.toUtf8(), url);
        job->addMetaData(QLatin1String("content-type"), QLatin1String("application/x-www-form-urlencoded"));
        job->addMetaData(QLatin1String("referrer"), page->mainFrame()->baseUrl().toString());
        connect(job, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibTeX(KJob *)));
        ++d->runningJobs;

        if (!d->articleUrls.isEmpty()) {
            QString url = d->articleUrls.first();
            d->articleUrls.removeFirst();
            page->mainFrame()->load(url);
            connect(page, SIGNAL(loadFinished(bool)), this, SLOT(doneFetchingArticlePage(bool)));
            ++d->runningJobs;
        }
        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else {
        kWarning() << "handleError returned false";
        cancel();
    }
}

void WebSearchSpringerLink::doneFetchingBibTeX(KJob * kJob)
{
    --d->runningJobs;

    if (handleErrors(kJob)) {
        KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob*>(kJob);
        QTextStream ts(job->data());
        QString bibTeXcode = ts.readAll();
        d->sanitizeBibTeXCode(bibTeXcode);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL)
                    emit foundEntry(entry);
            }
            delete bibtexFile;
        }

        if (d->runningJobs <= 0)
            emit stoppedSearch(resultNoError);
    } else {
        kWarning() << "handleError returned false";
        cancel();
    }
}