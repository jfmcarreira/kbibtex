/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_XSLTRANSFORM_H
#define KBIBTEX_XSLTRANSFORM_H

#include "kbibtexio_export.h"

#include <QString>

#include <libxslt/transform.h>

/**
 * This class is a wrapper around libxslt, which allows to
 * apply XSL transformation on XML files.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT XSLTransform
{
public:
    /**
     * Create a new instance of a transformer.
     * @param xsltFilename file name of the XSL file
     */
    static XSLTransform *createXSLTransform(const QString &xsltFilename);

    ~XSLTransform();

    /**
     * Transform a given XML document using the tranformer's
     * XSL file.
     * @param xmlText XML document to transform
     * @return transformed document
     */
    QString transform(const QString &xmlText) const;

    static void cleanupGlobals();

protected:
    XSLTransform(const xsltStylesheetPtr xsltStylesheet);

private:
    const xsltStylesheetPtr xsltStylesheet;
};

#endif // KBIBTEX_XSLTRANSFORM_H
