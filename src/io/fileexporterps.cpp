/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileexporterps.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDir>

#include <KBibTeX>
#include <Preferences>
#include <Element>
#include "fileexporterbibtex.h"
#include "logging_io.h"

FileExporterPS::FileExporterPS(QObject *parent)
        : FileExporterToolchain(parent)
{
    m_fileBasename = QStringLiteral("bibtex-to-ps");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;
}

FileExporterPS::~FileExporterPS()
{
    /// nothing
}

bool FileExporterPS::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, bibtexfile, errorLog);
        output.close();
    }

    if (result)
        result = generatePS(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterPS::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, element, bibtexfile, errorLog);
        output.close();
    }

    if (result)
        result = generatePS(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterPS::generatePS(QIODevice *iodevice, QStringList *errorLog)
{
    QStringList cmdLines {QStringLiteral("latex -halt-on-error bibtex-to-ps.tex"), QStringLiteral("bibtex bibtex-to-ps"), QStringLiteral("latex -halt-on-error bibtex-to-ps.tex"), QStringLiteral("latex -halt-on-error bibtex-to-ps.tex"), QStringLiteral("dvips -R2 -o bibtex-to-ps.ps bibtex-to-ps.dvi")};

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines, errorLog) && beautifyPostscriptFile(m_fileStem + KBibTeX::extensionPostScript, QStringLiteral("Exported Bibliography")) && writeFileToIODevice(m_fileStem + KBibTeX::extensionPostScript, iodevice, errorLog);
}

bool FileExporterPS::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}" << endl;
        ts << "\\usepackage[T1]{fontenc}" << endl;
        ts << "\\usepackage[utf8]{inputenc}" << endl;
        if (kpsewhich(QStringLiteral("babel.sty")))
            ts << "\\usepackage[" << Preferences::instance().laTeXBabelLanguage() << "]{babel}" << endl;
        if (kpsewhich(QStringLiteral("url.sty")))
            ts << "\\usepackage{url}" << endl;
        const QString bibliographyStyle = Preferences::instance().bibTeXBibliographyStyle();
        if (bibliographyStyle.startsWith(QStringLiteral("apacite")) && kpsewhich(QStringLiteral("apacite.sty")))
            ts << "\\usepackage[bibnewpage]{apacite}" << endl;
        if ((bibliographyStyle == QStringLiteral("agsm") || bibliographyStyle == QStringLiteral("dcu") || bibliographyStyle == QStringLiteral("jmr") || bibliographyStyle == QStringLiteral("jphysicsB") || bibliographyStyle == QStringLiteral("kluwer") || bibliographyStyle == QStringLiteral("nederlands") || bibliographyStyle == QStringLiteral("dcu") || bibliographyStyle == QStringLiteral("dcu")) && kpsewhich(QStringLiteral("harvard.sty")) && kpsewhich(QStringLiteral("html.sty")))
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
        if (kpsewhich(QStringLiteral("geometry.sty")))
            ts << "\\usepackage[paper=" << pageSizeToLaTeXName(Preferences::instance().pageSize()) << "]{geometry}" << endl;
        ts << "\\bibliographystyle{" << bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;
        ts << "\\nocite{*}" << endl;
        ts << "\\bibliography{bibtex-to-ps}" << endl;
        ts << "\\end{document}" << endl;
        latexFile.close();
        return true;
    } else
        return false;
}

bool FileExporterPS::beautifyPostscriptFile(const QString &filename, const QString &title)
{
    QFile postscriptFile(filename);
    if (postscriptFile.open(QFile::ReadOnly)) {
        QTextStream ts(&postscriptFile);
        QStringList lines;
        QString line;
        int i = 0;
        while (!(line = ts.readLine()).isNull()) {
            if (i < 32 && line.startsWith(QStringLiteral("%%Title:")))
                line = "%%Title: " + title;
            else if (i < 32 && line.startsWith(QStringLiteral("%%Creator:")))
                line += QStringLiteral("; exported from within KBibTeX: https://userbase.kde.org/KBibTeX");
            lines += line;
            ++i;
        }
        postscriptFile.close();

        if (postscriptFile.open(QFile::WriteOnly)) {
            QTextStream ts(&postscriptFile);
            for (const QString &line : const_cast<const QStringList &>(lines))
                ts << line << endl;
            postscriptFile.close();
        } else
            return false;
    } else
        return false;

    return true;
}
