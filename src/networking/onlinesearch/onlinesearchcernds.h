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

#ifndef KBIBTEX_ONLINESEARCH_CERNDS_H
#define KBIBTEX_ONLINESEARCH_CERNDS_H

#include "onlinesearchsimplebibtexdownload.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchCERNDS : public OnlineSearchSimpleBibTeXDownload
{
    Q_OBJECT

public:
    explicit OnlineSearchCERNDS(QWidget *parent);

    virtual QString label() const;
    virtual OnlineSearchQueryFormAbstract *customWidget(QWidget *parent);
    virtual KUrl homepage() const;

protected:
    virtual QString favIconUrl() const;
    virtual KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults);
};

#endif // KBIBTEX_ONLINESEARCH_CERNDS_H
