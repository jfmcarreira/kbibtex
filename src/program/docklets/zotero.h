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

#ifndef KBIBTEX_PROGRAM_ZOTERO_H
#define KBIBTEX_PROGRAM_ZOTERO_H

#include <QWidget>
#include <QModelIndex>

class BibTeXEditor;

class Zotero : public QWidget
{
    Q_OBJECT

public:
    Zotero(QWidget *parent);
    ~Zotero();

    void setEditor(BibTeXEditor *editor);

private slots:
    void changeBusyStatus(bool isBusy);
    void updateFilter(const QModelIndex &index);

private:
    class ZoteroPrivate;
    ZoteroPrivate *d;
};

#endif // KBIBTEX_PROGRAM_ZOTERO_H