/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#ifndef KBIBTEX_GUI_BIBTEXFIELDS_H
#define KBIBTEX_GUI_BIBTEXFIELDS_H

#include <QString>
#include <QList>

class KConfig;

namespace KBibTeX
{
namespace GUI {
namespace Config {

typedef struct {
    QString raw;
    QString label;
    int width;
} FieldDescription;

/**
@author Thomas Fischer
*/
class BibTeXFields : public QList<FieldDescription>
{
public:
    virtual ~BibTeXFields();

    static BibTeXFields *self();
    void load();
    void save();

protected:
    BibTeXFields();

    static BibTeXFields *m_self;

private:
    KConfig *m_config;
};
}
}
}

#endif // KBIBTEX_GUI_BIBTEXFIELDS_H
