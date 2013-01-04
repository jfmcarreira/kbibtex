/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>

#include <KDebug>

#include "fileimporterbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "onlinesearchsimplebibtexdownload.h"

OnlineSearchSimpleBibTeXDownload::OnlineSearchSimpleBibTeXDownload(QWidget *parent)
        : OnlineSearchAbstract(parent)
{
    // nothing
}

void OnlineSearchSimpleBibTeXDownload::startSearch()
{
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
}

void OnlineSearchSimpleBibTeXDownload::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QNetworkRequest request(buildQueryUrl(query, numResults));
    kDebug() << "request url=" << request.url().toString();
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, 2);
}

void OnlineSearchSimpleBibTeXDownload::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchSimpleBibTeXDownload::downloadDone()
{
    emit progress(1, 2);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        QTextStream ts(reply->readAll());
        ts.setCodec("utf-8");
        QString bibTeXcode = ts.readAll();

        if (bibTeXcode.contains(QLatin1String("<html")) || bibTeXcode.contains(QLatin1String("<HTML"))) {
            /// Replace all linebreak-like characters, in case they occur inside the BibTeX code
            static const QRegExp htmlLinebreakRegExp(QLatin1String("<[/]?(br|p)[^>]*[/]?>"));
            bibTeXcode = bibTeXcode.replace(htmlLinebreakRegExp, QLatin1String(""));

            /// Find first BibTeX entry in HTML code, clip away all HTML code before that
            static const QRegExp elementTypeRegExp(QLatin1String("[@]\\S+\\{"));
            int p1 = -1;
            /// hop over JavaScript's "@import" statements
            while ((p1 = bibTeXcode.indexOf(elementTypeRegExp, p1 + 1)) >= 0 && elementTypeRegExp.cap(0) == QLatin1String("@import{"));
            if (p1 > 1)
                bibTeXcode = bibTeXcode.mid(p1);

            /// Find HTML code after BibTeX code, clip that away, too
            static const QRegExp htmlContinuationRegExp(QLatin1String("<[/]?\\S+"));
            p1 = bibTeXcode.indexOf(htmlContinuationRegExp);
            if (p1 > 1)
                bibTeXcode = bibTeXcode.left(p1 - 1);
        }

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    if (!entry.isNull()) {
                        /// Sometimes, there is no identifier, so set a random one
                        if (entry->id().isEmpty())
                            entry->setId(QString(QLatin1String("entry-%1")).arg(QString::number(qrand(), 36)));
                        Value v;
                        v.append(QSharedPointer<VerbatimText>(new VerbatimText(label())));
                        entry->insert("x-fetchedfrom", v);
                        emit foundEntry(entry);
                        hasEntries = true;
                    }

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
    } else
        kDebug() << "url was" << reply->url().toString();

    emit progress(2, 2);
}