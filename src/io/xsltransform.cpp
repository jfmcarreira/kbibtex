/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2018 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "xsltransform.h"

#include <QFileInfo>
#include <QXmlQuery>
#include <QXmlSerializer>
#include <QBuffer>
#include <QCoreApplication>
#include <QStandardPaths>

#include "logging_io.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
XSLTransform::XSLTransform(const QString &xsltFilename)
        : xsltData(nullptr)
{
    if (!xsltFilename.isEmpty()) {
        QFile xsltFile(xsltFilename);
        if (xsltFile.open(QFile::ReadOnly)) {
            xsltData = new QByteArray(xsltFile.readAll());
            xsltFile.close();
            if (xsltData->size() == 0) {
                qCWarning(LOG_KBIBTEX_IO) << "Read only 0 Bytes from file" << xsltFilename;
                delete xsltData;
                xsltData = nullptr;
            }
        } else
            qCWarning(LOG_KBIBTEX_IO) << "Opening XSLT file" << xsltFilename << "failed";
    } else
        qCWarning(LOG_KBIBTEX_IO) << "Empty filename for XSLT";
}

XSLTransform::~XSLTransform() {
    if (xsltData != nullptr) delete xsltData;
}

bool XSLTransform::isValid() const
{
    return xsltData != nullptr;
}

QString XSLTransform::transform(const QString &xmlText) const
{
    if (xsltData == nullptr) {
        qCWarning(LOG_KBIBTEX_IO) << "Empty XSL transformation cannot transform";
        return QString();
    }

    QXmlQuery query(QXmlQuery::XSLT20);

    if (!query.setFocus(xmlText)) {
        qCWarning(LOG_KBIBTEX_IO) << "Invoking QXmlQuery::setFocus(" << xmlText.left(32) << "...) failed";
        return QString();
    }

    QBuffer xsltBuffer(xsltData);
    xsltBuffer.open(QBuffer::ReadOnly);
    query.setQuery(&xsltBuffer);

    if (!query.isValid()) {
        qCWarning(LOG_KBIBTEX_IO) << "QXmlQuery::isValid got negative result";
        return QString();
    }

    static QByteArray ba(1 << 24, '\0');
    QBuffer buffer(&ba);
    if (!buffer.open(QBuffer::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Could not open buffer to receive data";
        return QString();
    }

    QXmlSerializer serializer(query, &buffer);
    if (query.evaluateTo(&serializer)) {
        const int size_written_data = static_cast<int>(buffer.pos() & 0x7fffffff);
        buffer.close();
        /// Return result of XSL transformation and replace
        /// the usual XML suspects with its plain counterparts
        return QString::fromUtf8(ba.constData(), size_written_data);
    } else {
        qCWarning(LOG_KBIBTEX_IO) << "Invoking QXmlQuery::evaluateTo(...) failed";
        return QString();
    }
}

QString XSLTransform::locateXSLTfile(const QString &stem)
{
    const QString xsltFilename = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QCoreApplication::instance()->applicationName().remove(QStringLiteral("test")) + QLatin1Char('/') + stem);
    if (xsltFilename.isEmpty())
        qCWarning(LOG_KBIBTEX_IO) << "Generated XSLT filename is empty for stem " << stem;
    else if (!QFileInfo::exists(xsltFilename))
        qCWarning(LOG_KBIBTEX_IO) << "Generated XSLT filename " << xsltFilename << " refers to non-existing file";
    return xsltFilename;
}
