/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "findlocalpdf.h"

#include <QDirIterator>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegExp>
#include <QRegularExpression>
#include <QApplication>
#include <QTemporaryFile>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QList>
#include <QSet>
#include <QUrl>

#include <poppler-qt5.h>

#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "value.h"
#include "fileinfo.h"
#include "logging_networking.h"

#include "findpdf.h"

//#include <onlinesearchieeexplore.h>


class FindLocalPDF::Private
{
private:
    FindLocalPDF *p;

public:
    Entry entry;
    bool abort;
    QList<FindPDF::ResultItem> results;

    Private(FindLocalPDF *parent)
            : p(parent)
    {
        /// nothing
    }

    void findPDFLocally()
    {
        QString path(QDir::homePath());

        /// Generate a string which contains the title's beginning
        QString searchWords;
        if (entry.contains(Entry::ftTitle)) {
            const QStringList titleChunks = PlainTextValue::text(entry.value(Entry::ftTitle)).split(QStringLiteral(" "), QString::SkipEmptyParts);
            if (!titleChunks.isEmpty()) {
                searchWords = titleChunks[0];
                for (int i = 1; i < titleChunks.count() && searchWords.length() < 64; ++i)
                    searchWords += QStringLiteral(".*") + titleChunks[i];
            }
        }

        searchWords.replace("/", ".*");
        QRegExp regExpMatch(searchWords);
        regExpMatch.setCaseSensitivity(Qt::CaseInsensitive);

        QRegExp rxExt("\\b(pdf)\\b");
        QDirIterator it(path, QDir::NoFilter, QDirIterator::Subdirectories);
        while (it.hasNext() && !abort) {
            QFileInfo info(it.next());
            if (rxExt.indexIn(info.suffix()) >= 0)
            {
                Poppler::Document *doc = NULL;
                bool bMatched = false;

                //
                //                              if( doc )
                //                              {
                //                                  bMatched |= doc->title().contains(regExpMatch);
                //                              }
                bMatched |= info.fileName().contains(regExpMatch);

                if (bMatched)
                {
                    doc = Poppler::Document::load(info.absoluteFilePath());
                    FindPDF::ResultItem resultItem;
                    resultItem.url = QUrl(info.absoluteFilePath());
                    resultItem.tempFilename = NULL;
                    resultItem.downloadMode = FindPDF::URLonly;
                    resultItem.relevance = 1.0;
                    resultItem.textPreview = doc->info(QStringLiteral("Title")).simplified();

                    results << resultItem;


                }
                if (doc)
                    delete doc;
            }
        }
    }
};

FindLocalPDF::FindLocalPDF(FindPDF *parent)
        : QThread(parent), d(new Private(this))
{
    /// nothing
    connect(this, &FindLocalPDF::finished, parent, &FindPDF::appendLocalResults);
}

FindLocalPDF::~FindLocalPDF()
{

}

bool FindLocalPDF::search(const Entry &entry)
{
    d->results.clear();
    d->entry = entry;
    start();
    return true;
}

void FindLocalPDF::run()
{
    d->abort = false;
    d->findPDFLocally();
}

void FindLocalPDF::abort()
{
    d->abort = true;
}

QList<FindPDF::ResultItem> FindLocalPDF::results()
{
    return d->results;
}
