/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchinspirehep.h"

#include <KLocalizedString>

OnlineSearchInspireHep::OnlineSearchInspireHep(QObject *parent)
        : OnlineSearchSimpleBibTeXDownload(parent)
{
    /// nothing
}

QString OnlineSearchInspireHep::label() const
{
    return i18n("Inspire High-Energy Physics Literature Database");
}

QUrl OnlineSearchInspireHep::homepage() const
{
    return QUrl(QStringLiteral("https://inspirehep.net/"));
}

QUrl OnlineSearchInspireHep::buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults)
{
    static const QString typedSearch = QStringLiteral("%1 %2"); ///< no quotation marks for search term?

    const QStringList freeTextWords = splitRespectingQuotationMarks(query[QueryKey::FreeText]);
    const QStringList yearWords = splitRespectingQuotationMarks(query[QueryKey::Year]);
    const QStringList titleWords = splitRespectingQuotationMarks(query[QueryKey::Title]);
    const QStringList authorWords = splitRespectingQuotationMarks(query[QueryKey::Author]);

    /// append search terms
    QStringList queryFragments;
    queryFragments.reserve(freeTextWords.size() + yearWords.size() + titleWords.size() + authorWords.size());

    /// add words from "free text" field
    for (const QString &text : freeTextWords)
        queryFragments.append(typedSearch.arg(QStringLiteral("ft"), text));

    /// add words from "year" field
    for (const QString &text : yearWords)
        queryFragments.append(typedSearch.arg(QStringLiteral("d"), text));

    /// add words from "title" field
    for (const QString &text : titleWords)
        queryFragments.append(typedSearch.arg(QStringLiteral("t"), text));

    /// add words from "author" field
    for (const QString &text : authorWords)
        queryFragments.append(typedSearch.arg(QStringLiteral("a"), text));

    /// Build URL
    // TODO as of August 2020, there is no new API in place, but the old one
    // is still reachable at 'old.inspirehep.net'
    QString urlText = QStringLiteral("https://old.inspirehep.net/search?ln=en&ln=en&of=hx&action_search=Search&sf=&so=d&rm=&sc=0");
    /// Set number of expected results
    urlText.append(QString(QStringLiteral("&rg=%1")).arg(numResults));
    /// Append actual query
    urlText.append(QStringLiteral("&p="));
    urlText.append(queryFragments.join(QStringLiteral(" and ")));
    /// URL-encode text
    urlText = urlText.replace(QLatin1Char(' '), QStringLiteral("%20")).replace(QLatin1Char('"'), QStringLiteral("%22"));

    return QUrl(urlText);
}
