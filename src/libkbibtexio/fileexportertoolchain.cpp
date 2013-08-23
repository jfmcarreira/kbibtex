/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <stdlib.h>

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QRegExp>
#include <QTextStream>

#include <KLocale>

#include "fileexportertoolchain.h"

const QString FileExporterToolchain::keyBabelLanguage = QLatin1String("babelLanguage");
const QString FileExporterToolchain::defaultBabelLanguage = QLatin1String("english");
const QString FileExporterToolchain::keyBibliographyStyle = QLatin1String("bibliographyStyle");
const QString FileExporterToolchain::defaultBibliographyStyle = QLatin1String("plain");

FileExporterToolchain::FileExporterToolchain()
        : FileExporter(), m_errorLog(NULL)
{
    tempDir.setAutoRemove(true);
}

bool FileExporterToolchain::runProcesses(const QStringList &progs, QStringList *errorLog)
{
    bool result = true;
    int i = 0;

    emit progress(0, progs.size());
    for (QStringList::ConstIterator it = progs.constBegin(); result && it != progs.constEnd(); it++) {
        QCoreApplication::instance()->processEvents();
        QStringList args = (*it).split(' ');
        QString cmd = args.first();
        args.erase(args.begin());
        result &= runProcess(cmd, args, errorLog);
        emit progress(i++, progs.size());
    }
    QCoreApplication::instance()->processEvents();
    return result;
}

bool FileExporterToolchain::runProcess(const QString &cmd,  const QStringList &args, QStringList *errorLog)
{
    bool result = false;

    m_process = new QProcess();
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    /// avoid some paranoid security settings in BibTeX
    processEnvironment.insert("openout_any", "r");
    /// Make applications use working directory as temporary directory
    processEnvironment.insert("TMPDIR", tempDir.name());
    processEnvironment.insert("TEMPDIR", tempDir.name());
    m_process->setProcessEnvironment(processEnvironment);
    m_process->setWorkingDirectory(tempDir.name());

    if (m_errorLog != NULL) {
        connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(slotReadProcessStandardOutput()));
        connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(slotReadProcessErrorOutput()));
    }

    if (errorLog != NULL)
        errorLog->append(i18n("Running process '%1' using working directory '%2'", (cmd + " " + args.join(" ")), m_process->workingDirectory()));
    m_process->start(cmd, args);
    m_errorLog = errorLog;

    if (m_process->waitForStarted(3000)) {
        if (m_process->waitForFinished(30000))
            result = m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0;
        else
            result = false;
    } else
        result = false;

    if (!result)
        errorLog->append(i18n("Process '%1' failed", (cmd + " " + args.join(" "))));

    if (errorLog != NULL) {
        QTextStream tsStdOut(m_process->readAllStandardOutput());
        QString line;
        while (!(line = tsStdOut.readLine()).isNull())
            m_errorLog->append(line);
        QTextStream tsStdErr(m_process->readAllStandardError());
        while (!(line = tsStdErr.readLine()).isNull())
            m_errorLog->append(line);

        errorLog->append(i18n("Stopped process '%1' with exit code %2", (cmd + " " + args.join(" ")), m_process->exitCode()));
    }

    delete(m_process);
    m_process = NULL;

    return result;
}

bool FileExporterToolchain::writeFileToIODevice(const QString &filename, QIODevice *device, QStringList *errorLog)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        bool result = true;
        qint64 buffersize = 0x10000;
        qint64 amount = 0;
        char *buffer = new char[ buffersize ];
        do {
            result = ((amount = file.read(buffer, buffersize)) > -1) && (device->write(buffer, amount) > -1);
        } while (result && amount > 0);

        file.close();
        delete[] buffer;

        if (errorLog != NULL)
            errorLog->append(i18n("Writing to file '%1'' succeeded", filename));
        return result;
    }

    if (errorLog != NULL)
        errorLog->append(i18n("Writing to file '%1'' failed", filename));
    return false;
}

void FileExporterToolchain::cancel()
{
    if (m_process != NULL) {
        qWarning("Canceling process");
        m_process->terminate();
        m_process->kill();
    }
}

void FileExporterToolchain::slotReadProcessStandardOutput()
{
    if (m_process) {
        QTextStream ts(m_process->readAllStandardOutput());
        QString line;
        while (!(line = ts.readLine()).isNull())
            m_errorLog->append(line);
    }
}

void FileExporterToolchain::slotReadProcessErrorOutput()
{
    if (m_process) {
        QTextStream ts(m_process->readAllStandardError());
        QString line;
        while (!(line = ts.readLine()).isNull())
            m_errorLog->append(line);
    }
}

bool FileExporterToolchain::kpsewhich(const QString &filename)
{
    bool result = false;

    QProcess kpsewhich;
    QStringList param;
    param << filename;
    kpsewhich.start("kpsewhich", param);

    if (kpsewhich.waitForStarted(3000)) {
        if (kpsewhich.waitForFinished(30000))
            result = kpsewhich.exitStatus() == QProcess::NormalExit;
        else
            result = false;
    } else
        result = false;

    return result;
}
