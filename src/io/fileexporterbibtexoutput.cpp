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

#include "fileexporterbibtexoutput.h"

#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QUrl>
#include <QTextStream>

#include <KBibTeX>
#include <Preferences>
#include <Element>
#include <Entry>
#include "fileexporterbibtex.h"
#include "logging_io.h"

FileExporterBibTeXOutput::FileExporterBibTeXOutput(OutputType outputType, QObject *parent)
        : FileExporterToolchain(parent), m_outputType(outputType)
{
    m_fileBasename = QStringLiteral("bibtex-to-output");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;
}

FileExporterBibTeXOutput::~FileExporterBibTeXOutput()
{
    /// nothing
}

bool FileExporterBibTeXOutput::save(QIODevice *ioDevice, const File *bibtexfile)
{
    if (!ioDevice->isWritable() && !ioDevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile bibTeXFile(m_fileStem + KBibTeX::extensionBibTeX);
    if (bibTeXFile.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("utf-8"));
        result = bibtexExporter.save(&bibTeXFile, bibtexfile);
        bibTeXFile.close();
    }

    if (result)
        result = generateOutput();

    if (result)
        result = writeFileToIODevice(m_fileStem + (m_outputType == OutputType::BibTeXLogFile ? KBibTeX::extensionBLG : KBibTeX::extensionBBL), ioDevice);

    return result;
}

bool FileExporterBibTeXOutput::save(QIODevice *ioDevice, const QSharedPointer<const Element> element, const File *bibtexfile)
{
    if (!ioDevice->isWritable() && !ioDevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile bibTeXFile(m_fileStem + KBibTeX::extensionBibTeX);
    if (bibTeXFile.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("utf-8"));
        result = bibtexExporter.save(&bibTeXFile, element, bibtexfile);
        bibTeXFile.close();
    }

    if (result)
        result = generateOutput();

    if (result)
        result = writeFileToIODevice(m_fileStem + (m_outputType == OutputType::BibTeXLogFile ? KBibTeX::extensionBLG : KBibTeX::extensionBBL), ioDevice);

    return result;
}

bool FileExporterBibTeXOutput::generateOutput()
{
    const QStringList cmdLinesBeforeBibTeX {QStringLiteral("pdflatex -halt-on-error ") + m_fileBasename + KBibTeX::extensionTeX};
    const QStringList bibTeXarguments {m_fileBasename + KBibTeX::extensionAux};
    const QStringList cmdLinesAfterBibTeX {};

    if (writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLinesBeforeBibTeX) && runProcess(QStringLiteral("bibtex"), bibTeXarguments, true) && runProcesses(cmdLinesAfterBibTeX))
        return true;
    else {
        qCWarning(LOG_KBIBTEX_IO) << "Generating BibTeX output failed";
        return false;
    }
}

bool FileExporterBibTeXOutput::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}\n";
        ts << "\\usepackage[T1]{fontenc}\n";
        ts << "\\usepackage[utf8]{inputenc}\n";
        if (kpsewhich(QStringLiteral("babel.sty")))
            ts << "\\usepackage[" << Preferences::instance().laTeXBabelLanguage() << "]{babel}\n";
        if (kpsewhich(QStringLiteral("hyperref.sty")))
            ts << "\\usepackage[pdfproducer={KBibTeX: https://userbase.kde.org/KBibTeX},pdftex]{hyperref}\n";
        else if (kpsewhich(QStringLiteral("url.sty")))
            ts << "\\usepackage{url}\n";
        const QString latexBibStyle = Preferences::instance().bibTeXBibliographyStyle();
        if (latexBibStyle.startsWith(QStringLiteral("apacite")) && kpsewhich(QStringLiteral("apacite.sty")))
            ts << "\\usepackage[bibnewpage]{apacite}\n";
        ts << "\\bibliographystyle{" << latexBibStyle << "}\n";
        ts << "\\begin{document}\n";
        ts << "\\nocite{*}\n";
        ts << QStringLiteral("\\bibliography{") + m_fileBasename + QStringLiteral("}\n");
        ts << "\\end{document}\n";
        latexFile.close();
        return true;
    } else
        return false;
}
