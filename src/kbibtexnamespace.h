/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#ifndef KBIBTEX_NAMESPACE_H
#define KBIBTEX_NAMESPACE_H

#include <QVariant>

namespace KBibTeX
{

enum Casing {
    cLowerCase,
    cInitialCapital,
    cUpperCamelCase,
    cLowerCamelCase,
    cUpperCase
};

enum FieldInputType {
    SingleLine = 1,
    MultiLine = 2,
    List = 3,
    URL = 4,
    Month = 5
};

enum TypeFlag {
    tfText = 0x1,
    tfReference = 0x2,
    tfPerson = 0x4,
    tfKeyword = 0x8,
    tfSource = 0x100
};
Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(TypeFlags)

// Q_DECLARE_METATYPE(TypeFlags);

static const QString MonthsTriple[] = {
    QLatin1String("jan"), QLatin1String("feb"), QLatin1String("mar"), QLatin1String("apr"), QLatin1String("may"), QLatin1String("jun"), QLatin1String("jul"), QLatin1String("aug"), QLatin1String("sep"), QLatin1String("oct"), QLatin1String("nov"), QLatin1String("dec")
};


}

#endif // KBIBTEX_NAMESPACE_H
