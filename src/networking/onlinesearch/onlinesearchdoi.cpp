/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchdoi.h"

#include <QLabel>
#include <QGridLayout>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <KLineEdit>
#include <KDebug>
#include <KConfigGroup>
#include <KLocale>

#include "kbibtexnamespace.h"
#include "internalnetworkaccessmanager.h"
#include "fileimporterbibtex.h"

class OnlineSearchDOI::OnlineSearchQueryFormDOI : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditDoiNumber->setText(configGroup.readEntry(QLatin1String("doiNumber"), QString()));
    }

public:
    KLineEdit *lineEditDoiNumber;

    OnlineSearchQueryFormDOI(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QLatin1String("Search Engine DOI")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("DOI:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditDoiNumber = new KLineEdit(this);
        layout->addWidget(lineEditDoiNumber, 0, 1, 1, 1);
        lineEditDoiNumber->setClearButtonShown(true);
        connect(lineEditDoiNumber, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        layout->setRowStretch(1, 100);
        lineEditDoiNumber->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const {
        return !lineEditDoiNumber->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) {
        lineEditDoiNumber->setText(PlainTextValue::text(entry[Entry::ftDOI]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("doiNumber"), lineEditDoiNumber->text());
        config->sync();
    }
};


class OnlineSearchDOI::OnlineSearchDOIPrivate
{
private:
    OnlineSearchDOI *p;

public:
    OnlineSearchQueryFormDOI *form;
    int numSteps, curStep;

    OnlineSearchDOIPrivate(OnlineSearchDOI *parent)
            : p(parent), form(NULL) {
        // nothing
    }

    KUrl buildQueryUrl() {
        if (form == NULL) {
            kWarning() << "Cannot build query url if no form is specified";
            return KUrl();
        }

        return KUrl(QLatin1String("http://dx.doi.org/") + form->lineEditDoiNumber->text());
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        if (KBibTeX::doiRegExp.indexIn(query[queryKeyFreeText]) >= 0) {
            return KUrl(QLatin1String("http://dx.doi.org/") + KBibTeX::doiRegExp.cap(0));
        }

        return KUrl();
    }
};


OnlineSearchDOI::OnlineSearchDOI(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchDOIPrivate(this))
{
    // TODO
}

OnlineSearchDOI::~OnlineSearchDOI()
{
    delete d;
}

void OnlineSearchDOI::startSearch()
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    const KUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

        emit progress(0, d->numSteps);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);
}

void OnlineSearchDOI::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    const KUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

        emit progress(0, d->numSteps);
    } else
        delayedStoppedSearch(resultNoError);
}

QString OnlineSearchDOI::label() const
{
    return i18n("DOI");
}

OnlineSearchQueryFormAbstract *OnlineSearchDOI::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchDOI::OnlineSearchQueryFormDOI(parent);
    return d->form;
}

KUrl OnlineSearchDOI::homepage() const
{
    return KUrl(QLatin1String("http://dx.doi.org/"));
}

void OnlineSearchDOI::cancel()
{
    OnlineSearchAbstract::cancel();
}

QString OnlineSearchDOI::favIconUrl() const
{
    return QLatin1String("http://dx.doi.org/favicon.ico");
}

void OnlineSearchDOI::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    KUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(downloadDone()));
        } else {  /// ensure proper treatment of UTF-8 characters
            const QString bibTeXcode = QString::fromUtf8(reply->readAll().data());

            if (!bibTeXcode.isEmpty()) {
                FileImporterBibTeX importer;
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != NULL) {
                    for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                        QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    if (!hasEntries)
                        kDebug() << "No hits found in" << reply->url().toString();
                    emit stoppedSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                    emit stoppedSearch(resultUnspecifiedError);
                }
            } else {
                /// returned file is empty
                kDebug() << "No hits found in" << reply->url().toString();
                emit stoppedSearch(resultNoError);
            }
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}