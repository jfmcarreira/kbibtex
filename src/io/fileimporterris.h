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
#ifndef KBIBTEX_IO_FILEIMPORTERRIS_H
#define KBIBTEX_IO_FILEIMPORTERRIS_H

#include "entry.h"
#include "fileimporter.h"

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileImporterRIS : public FileImporter
{
    Q_OBJECT

public:
    FileImporterRIS();
    ~FileImporterRIS();

    File *load(QIODevice *iodevice);
    static bool guessCanDecode(const QString &text);

public slots:
    void cancel();

private:
    class FileImporterRISPrivate;
    FileImporterRISPrivate *d;
};

#endif // KBIBTEX_IO_FILEIMPORTERRIS_H
