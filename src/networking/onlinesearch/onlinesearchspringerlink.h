/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#ifndef KBIBTEX_ONLINESEARCH_SPRINGERLINK_H
#define KBIBTEX_ONLINESEARCH_SPRINGERLINK_H

#include "onlinesearchabstract.h"


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *
 * See also: http://dev.springer.com/
 *
 * On the subject of having multiple "constraints" (search terms) in
 * a search, Springer's documentation states: "Each constraint that
 * appears in your request will automatically be ANDed with all the others
 * For instance, a request including constraints: "title:bone+name:Jones"
 * is the equivilent to the request containing constraints concatenated by
 * the AND operator: "title:bone%20AND%20name:Jones".
 * (source: http://dev.springer.com/docs/read/Filters_Facets_and_Constraints)
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchSpringerLink : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    OnlineSearchSpringerLink(QWidget *parent);
    ~OnlineSearchSpringerLink();

    virtual void startSearch();
    virtual void startSearch(const QMap<QString, QString> &query, int numResults);
    virtual QString label() const;
    virtual OnlineSearchQueryFormAbstract *customWidget(QWidget *parent);
    virtual KUrl homepage() const;

public slots:
    void cancel();

protected:
    virtual QString favIconUrl() const;

private slots:
    void doneFetchingPAM();

private:
    class OnlineSearchQueryFormSpringerLink;

    class OnlineSearchSpringerLinkPrivate;
    OnlineSearchSpringerLinkPrivate *d;
};

#endif // KBIBTEX_ONLINESEARCH_SPRINGERLINK_H
