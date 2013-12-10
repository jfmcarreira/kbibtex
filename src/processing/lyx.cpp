/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <sys/stat.h>

#include <QWidget>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>

#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KParts/ReadOnlyPart>
#include <KMessageBox>
#include <KStandardDirs>
#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>

#include <kdeversion.h>

#include "lyx.h"

class LyX::LyXPrivate
{
private:
    LyX *p;

public:
    QWidget *widget;
    KAction *action;
    QStringList references;

    KSharedConfigPtr config;
    const KConfigGroup group;

    LyXPrivate(LyX *parent, QWidget *widget)
            : p(parent), action(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), group(config, LyX::configGroupName) {
        this->widget = widget;
    }

    QString locateConfiguredLyXPipe() {
        QString result;

        /// First, check if automatic detection is disabled.
        /// In this case, read the LyX pipe's path from configuration
        if (!group.readEntry(keyUseAutomaticLyXPipeDetection, defaultUseAutomaticLyXPipeDetection))
            result = group.readEntry(keyLyXPipePath, defaultLyXPipePath);

        /// Check if the result so far is empty. This means that
        /// either automatic detection is enabled or the path in
        /// the configuration is empty/invalid. Proceed with
        /// automatic detection in this case.
        if (result.isEmpty())
            result = LyX::guessLyXPipeLocation();

        /// Finally, even if automatic detection was preferred by the user,
        /// still check configuration for a path if automatic detection failed
        if (result.isEmpty() && group.readEntry(keyUseAutomaticLyXPipeDetection, defaultUseAutomaticLyXPipeDetection))
            result = group.readEntry(keyLyXPipePath, defaultLyXPipePath);

        /// Return the best found LyX pipe path
        return result;
    }
};

const QString LyX::keyUseAutomaticLyXPipeDetection = QLatin1String("UseAutomaticLyXPipeDetection");
const QString LyX::keyLyXPipePath = QLatin1String("LyXPipePath");
const bool LyX::defaultUseAutomaticLyXPipeDetection =
#ifdef Q_WS_WIN
    false; /// Windows is not supported yet
#else // Q_WS_WIN
    true;
#endif // Q_WS_WIN
const QString LyX::defaultLyXPipePath = QString();
const QString LyX::configGroupName = QLatin1String("LyXPipe");


LyX::LyX(KParts::ReadOnlyPart *part, QWidget *widget)
        : QObject(part), d(new LyX::LyXPrivate(this, widget))
{
    d->action = new KAction(KIcon("application-x-lyx"), i18n("Send to LyX/Kile"), this);
    part->actionCollection()->addAction("sendtolyx", d->action);
    d->action->setEnabled(false);
    connect(d->action, SIGNAL(triggered()), this, SLOT(sendReferenceToLyX()));
#if KDE_VERSION_MINOR >= 4
    part->replaceXMLFile(KStandardDirs::locate("data", "kbibtex/lyx.rc"), KStandardDirs::locateLocal("data", "kbibtex/lyx.rc"), true);
#endif
    widget->addAction(d->action);
}

LyX::~LyX()
{
    delete d;
}

void LyX::setReferences(const QStringList &references)
{
    d->references = references;
    d->action->setEnabled(d->widget != NULL && !d->references.isEmpty());
}

void LyX::sendReferenceToLyX()
{
    const QString defaultHintOnLyXProblems = i18n("\n\nCheck that LyX or Kile are running and configured to receive references.");
    const QString msgBoxTitle = i18n("Send Reference to LyX");
    /// LyX pipe name has to determined always fresh in case LyX or Kile exited
    const QString pipeName = d->locateConfiguredLyXPipe();

    if (pipeName.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No 'LyX server pipe' was detected.") + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    if (d->references.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No references to send to LyX/Kile."), msgBoxTitle);
        return;
    }

    QFile pipe(pipeName);
    if (!QFileInfo(pipeName).exists() || !pipe.open(QFile::WriteOnly)) {
        KMessageBox::error(d->widget, i18n("Could not open LyX server pipe '%1'.", pipeName) + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    QTextStream ts(&pipe);
    QString msg = QString("LYXCMD:kbibtex:citation-insert:%1").arg(d->references.join(","));

    ts << msg << endl;
    ts.flush();

    pipe.close();
}

QString LyX::guessLyXPipeLocation()
{
    struct stat fileInfo;
    const QStringList nameFilter = QStringList() << QLatin1String("*lyxpipe*in*");
    QString result;

    /// Start with scanning the user's home directory for pipes
    QDir home = QDir::home();
    QStringList files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
    foreach(const QString &filename, files) {
        QString const absoluteFilename = home.absolutePath() + QDir::separator() + filename;
        if (stat(absoluteFilename.toLatin1(), &fileInfo) == 0 && S_ISFIFO(fileInfo.st_mode)) {
            result = absoluteFilename;
            break;
        }
    }

    /// No hit yet? Search LyX's configuration directory
    if (result.isEmpty()) {
        QDir home = QDir::home();
        if (home.cd(QLatin1String(".lyx"))) {
            /// Same search again here
            QStringList files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
            foreach(const QString &filename, files) {
                QString const absoluteFilename = home.absolutePath() + QDir::separator() + filename;
                if (stat(absoluteFilename.toLatin1(), &fileInfo) == 0 && S_ISFIFO(fileInfo.st_mode)) {
                    result = absoluteFilename;
                    break;
                }
            }
        }
    }

    return result;
}
