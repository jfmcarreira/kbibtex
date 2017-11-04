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

#ifndef KBIBTEX_NETWORKING_FINDPDFLOCALLY_H
#define KBIBTEX_NETWORKING_FINDPDFLOCALLY_H

#include "kbibtexnetworking_export.h"

#include <QThread>
#include <QList>

#include "entry.h"
#include "findpdf.h"

/**
 * Search for PDF files in local directory
 *
 * @author Joao Carreira <jfmcarreira@gmail.com>
 */
class KBIBTEXNETWORKING_EXPORT FindLocalPDF : public QThread
{
    Q_OBJECT

public:
    FindLocalPDF(FindPDF *parent);
    ~FindLocalPDF();

    bool search(const Entry &entry);
    QList<FindPDF::ResultItem> results();

public slots:
    /**
     * Abort any running downloads.
     */
    void abort();

protected:
    void run();
private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_NETWORKING_FINDPDFLOCALLY_H
