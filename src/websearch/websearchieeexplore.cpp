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

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QTextStream>

#include <KMessageBox>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <kio/job.h>
#include <KStandardDirs>

#include "websearchieeexplore.h"
#include "xsltransform.h"
#include "fileimporterbibtex.h"

class WebSearchIEEEXplore::WebSearchQueryFormIEEEXplore : public WebSearchQueryFormAbstract
{
public:
    WebSearchQueryFormIEEEXplore(QWidget *parent)
            : WebSearchQueryFormAbstract(parent) {
        // TODO
    }

    bool readyToStart() const {
        // TODO
        return false;
    }
};

class WebSearchIEEEXplore::WebSearchIEEEXplorePrivate
{
private:
    WebSearchIEEEXplore *p;
    QMap<QString, QString> originalCookiesSettings;
    bool originalCookiesEnabled;

public:
    QWidget *w;
    bool hasBeenCancelled;
    KIO::TransferJob *currentJob;
    WebSearchQueryFormIEEEXplore *form;
    KIO::StoredTransferJob *job;
    int numResults;
    QStringList queryFragments;
    QStringList arnumberList;
    QString startPageUrl, searchRequestUrl, fullAbstractUrl, citationUrl, citationPostData;
    FileImporterBibTeX fileImporter;

    WebSearchIEEEXplorePrivate(QWidget *widget, WebSearchIEEEXplore *parent)
            : p(parent), w(widget), hasBeenCancelled(false), currentJob(NULL), form(NULL), job(NULL) {
        startPageUrl = QLatin1String("http://ieeexplore.ieee.org/");
        searchRequestUrl = QLatin1String("http://ieeexplore.ieee.org/search/searchresult.jsp?newsearch=true&x=0&y=0&queryText=");
        fullAbstractUrl = QLatin1String("http://ieeexplore.ieee.org/search/srchabstract.jsp?tp=&arnumber=");
        citationUrl = QLatin1String("http://ieeexplore.ieee.org/xpl/downloadCitations?fromPageName=searchabstract&citations-format=citation-abstract&download-format=download-bibtex&x=61&y=24&recordIds=");
    }

    bool cookiesEnabled() {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        QString answer = group.readEntry("Cookies", "false");
        return answer.toLower() == "true";
    }

    void setCookiesEnabled(bool enabled) {
        QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
        QDBusReply<bool> reply = kded.call("loadModule", QString("kcookiejar"));

        KConfig cfg("kcookiejarrc");
        KConfigGroup group = cfg.group("Cookie Policy");
        group.writeEntry("Cookies", enabled ? "true" : "false");

        if (enabled) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            kcookiejar.call("reloadPolicy");
        } else {
            QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
            kded.call("shutdown");
        }
    }

    void changeCookieSettings(const QString &url) {
        if (originalCookiesSettings.contains(url))
            return;

        bool success = false;
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        QDBusReply<QString> replyGetDomainAdvice = kcookiejar.call("getDomainAdvice", url);
        if (replyGetDomainAdvice.isValid()) {
            originalCookiesSettings.insert(url, replyGetDomainAdvice.value());
            QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", url, "accept");
            success = replySetDomainAdvice.isValid() && replySetDomainAdvice.value();
            if (success) {
                QDBusReply<void> reply = kcookiejar.call("reloadPolicy");
                success = reply.isValid();
            }
        }

        if (!success) {
            restoreOldCookieSettings();
            p->doStopSearch(resultUnspecifiedError);
            KMessageBox::error(w, i18n("Could not change cookie policy for url \"%1\".\n\nIt is necessary that cookies are accepted from this source.", url));
            return;
        }
    }

    void initializeCookieSettings() {
        originalCookiesEnabled = cookiesEnabled();
        setCookiesEnabled(true);
        originalCookiesSettings.clear();
        changeCookieSettings(startPageUrl);
    }

    void restoreOldCookieSettings() {
        QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
        for (QMap<QString, QString>::ConstIterator it = originalCookiesSettings.constBegin(); it != originalCookiesSettings.constEnd(); ++it)
            QDBusReply<bool> replySetDomainAdvice = kcookiejar.call("setDomainAdvice", it.key(), it.value());

        setCookiesEnabled(originalCookiesEnabled);
        kcookiejar.call("reloadPolicy");
        originalCookiesSettings.clear();
    }

    void startSearch() {
        hasBeenCancelled = false;
        currentJob = NULL;

        initializeCookieSettings();

        KIO::TransferJob *job = KIO::get(startPageUrl, KIO::Reload);
        job->addMetaData("cookies", "auto");
        job->addMetaData("cache", "reload");
        connect(job, SIGNAL(result(KJob *)), p, SLOT(doneFetchingStartPage(KJob*)));
        connect(job, SIGNAL(redirection(KIO::Job*, KUrl)), p, SLOT(redirection(KIO::Job*, KUrl)));
        connect(job, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), p, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        currentJob = job;
    }

    void sanitize(Entry *entry, const QString &arnumber) {
        entry->setId(QLatin1String("ieee") + arnumber);

        Value v;
        v << new PlainText(arnumber);
        entry->insert(QLatin1String("arnumber"), v);
    }
};

WebSearchIEEEXplore::WebSearchIEEEXplore(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchIEEEXplore::WebSearchIEEEXplorePrivate(parent, this))
{
    // nothing
}

void WebSearchIEEEXplore::startSearch()
{
    // TODO: No customized search widget
}

void WebSearchIEEEXplore::startSearch(const QMap<QString, QString> &query, int numResults)
{
    d->numResults = numResults;

    d->queryFragments.clear();
    for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
        // FIXME: Is there a need for percent encoding?
        d->queryFragments.append(splitRespectingQuotationMarks(it.value()));
    }

    d->startSearch();
}

void WebSearchIEEEXplore::doneFetchingStartPage(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        KMessageBox::error(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    QString url = d->searchRequestUrl + '"' + d->queryFragments.join("\"+AND+\"") + '"';
    kDebug() << "url = " << url;
    KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
    newJob->addMetaData("cookies", "auto");
    newJob->addMetaData("cache", "reload");
    connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingSearchResults(KJob*)));
    connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
    connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
    d->currentJob = newJob;
}

void WebSearchIEEEXplore::doneFetchingSearchResults(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        KMessageBox::error(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);

    QString htmlText(transferJob->data());
    QRegExp arnumberRegExp("arnumber=(\\d+)[^0-9]");
    d->arnumberList.clear();
    int p = -1;
    while ((p = arnumberRegExp.indexIn(htmlText, p + 1)) >= 0) {
        QString arnumber = arnumberRegExp.cap(1);
        if (!d->arnumberList.contains(arnumber))
            d->arnumberList << arnumber;
        if (d->arnumberList.count() >= d->numResults)
            break;
    }
    kDebug() << "arnumberList = " << d->arnumberList.join(",");

    if (d->arnumberList.isEmpty()) {
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultNoError);
        return;
    } else {
        QString url = d->fullAbstractUrl + d->arnumberList.first();
        kDebug() << "url = " << url;
        KIO::TransferJob * newJob = KIO::get(url, KIO::Reload);
        d->arnumberList.removeFirst();
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingAbstract(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    }
}

void WebSearchIEEEXplore::doneFetchingAbstract(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        KMessageBox::error(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::TransferJob *transferJob = static_cast<KIO::TransferJob *>(kJob);
    QString arnumber = transferJob->url().queryItemValue(QLatin1String("arnumber"));
    if (!arnumber.isEmpty()) {
        QString url = d->citationUrl + arnumber;
        kDebug() << "url = " << url << "  arnumber=" << arnumber;
        KIO::StoredTransferJob * newJob = KIO::storedGet(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingBibliography(KJob*)));
        d->currentJob = newJob;
    }
}

void WebSearchIEEEXplore::doneFetchingBibliography(KJob *kJob)
{
    d->currentJob = NULL;
    if (d->hasBeenCancelled) {
        kWarning() << "Searching " << label() << " got cancelled";
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultCancelled);
        return;
    } else if (kJob->error() != KJob::NoError) {
        kWarning() << "Searching " << label() << " failed with error message: " << kJob->errorString();
        KMessageBox::error(d->w, i18n("Searching \"%1\" failed with error message:\n\n%2", label(), kJob->errorString()));
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    KIO::StoredTransferJob *transferJob = static_cast<KIO::StoredTransferJob *>(kJob);
    QString plainText = QString(transferJob->data()).replace("<br>", "");

    File *bibtexFile = d->fileImporter.fromString(plainText);
    Entry *entry = NULL;
    if (bibtexFile != NULL) {
        for (File::ConstIterator it = bibtexFile->constBegin(); entry == NULL && it != bibtexFile->constEnd(); ++it) {
            entry = dynamic_cast<Entry*>(*it);
            if (entry != NULL) {
                QString arnumber = transferJob->url().queryItemValue(QLatin1String("recordIds"));
                d->sanitize(entry, arnumber);
                emit foundEntry(entry);
            }
        }
        delete bibtexFile;
    }

    if (entry == NULL) {
        kWarning() << "Searching " << label() << " resulted in invalid BibTeX data: " << QString(transferJob->data());
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultUnspecifiedError);
        return;
    }

    if (!d->arnumberList.isEmpty()) {
        QString url = d->fullAbstractUrl + d->arnumberList.first();
        d->arnumberList.removeFirst();
        kDebug() << "url = " << url;
        KIO::TransferJob * newJob = KIO::get(url, KIO::Reload);
        newJob->addMetaData("cookies", "auto");
        newJob->addMetaData("cache", "reload");
        connect(newJob, SIGNAL(result(KJob *)), this, SLOT(doneFetchingAbstract(KJob*)));
        connect(newJob, SIGNAL(redirection(KIO::Job*, KUrl)), this, SLOT(redirection(KIO::Job*, KUrl)));
        connect(newJob, SIGNAL(permanentRedirection(KIO::Job*, KUrl, KUrl)), this, SLOT(permanentRedirection(KIO::Job*, KUrl, KUrl)));
        d->currentJob = newJob;
    } else {
        d->restoreOldCookieSettings();
        emit stoppedSearch(resultNoError);
    }
}

void WebSearchIEEEXplore::permanentRedirection(KIO::Job *, const KUrl &, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->changeCookieSettings(url);
}

void WebSearchIEEEXplore::redirection(KIO::Job *, const KUrl &u)
{
    QString url = "http://" + u.host();
    d->changeCookieSettings(url);
}

QString WebSearchIEEEXplore::label() const
{
    return i18n("IEEEXplore");
}

QString WebSearchIEEEXplore::favIconUrl() const
{
    return QLatin1String("http://ieeexplore.ieee.org/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchIEEEXplore::customWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    // TODO: No customized search widget
    // return (d->form = new WebSearchIEEEXplore::WebSearchQueryFormIEEEXplore(parent));
    return NULL;
}

KUrl WebSearchIEEEXplore::homepage() const
{
    return KUrl("http://ieeexplore.ieee.org/");
}

void WebSearchIEEEXplore::cancel()
{
    if (d->job != NULL)
        d->job->kill(KJob::EmitResult);
}

void WebSearchIEEEXplore::doStopSearch(int e)
{
    emit stoppedSearch(e);
}
