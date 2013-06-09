/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#ifndef KBIBTEX_PROGRAM_OPENFILEINFO_H
#define KBIBTEX_PROGRAM_OPENFILEINFO_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QVariant>

#include <KService>
#include <KUrl>

#include "fileinfo.h"

namespace KParts
{
class ReadOnlyPart;
}

class OpenFileInfoManager;

class OpenFileInfo : public QObject
{
    Q_OBJECT

public:
    enum StatusFlag {
        Open = 0x1,
        RecentlyUsed = 0x2,
        Favorite = 0x4,
        HasName = 0x8
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    ~OpenFileInfo();

    KParts::ReadOnlyPart *part(QWidget *parent, KService::Ptr servicePtr = KService::Ptr());

    QString shortCaption() const;
    QString fullCaption() const;
    QString mimeType() const;
    KUrl url() const;
    bool isModified() const;
    bool save();

    /**
     * Close the current file. The user may interrupt the closing
     * if the file is modified and he/she presses "Cancel" when asked
     * to close the modified file.
     *
     * @return returns true if the closing was successful, otherwise false
     */
    bool close();

    StatusFlags flags() const;
    void setFlags(StatusFlags statusFlags);
    void addFlags(StatusFlags statusFlags);
    void removeFlags(StatusFlags statusFlags);

    QDateTime lastAccess() const;
    void setLastAccess(const QDateTime &dateTime = QDateTime::currentDateTime());

    KService::List listOfServices();
    KService::Ptr defaultService();
    KService::Ptr currentService();

    friend class OpenFileInfoManager;

signals:
    void flagsChanged(OpenFileInfo::StatusFlags statusFlags);

protected:
    OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const KUrl &url);
    OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType = FileInfo::mimetypeBibTeX);
    void setUrl(const KUrl &url);

private:
    class OpenFileInfoPrivate;
    OpenFileInfoPrivate *d;
};

Q_DECLARE_METATYPE(OpenFileInfo *);


class OpenFileInfoManager: public QObject
{
    Q_OBJECT

public:
    typedef QVector<OpenFileInfo *> OpenFileInfoList;

    OpenFileInfoManager(QWidget *parent);
    ~OpenFileInfoManager();

    OpenFileInfo *createNew(const QString &mimeType = FileInfo::mimetypeBibTeX);
    OpenFileInfo *open(const KUrl &url);
    OpenFileInfo *contains(const KUrl &url) const;
    OpenFileInfo *currentFile() const;
    bool changeUrl(OpenFileInfo *openFileInfo, const KUrl &url);
    bool close(OpenFileInfo *openFileInfo);

    /**
     * Try to close all open files. If a file is modified, the user will be asked
     * if the file shall be saved first. Depending on the KPart, the user may opt
     * to cancel the closing operation for any modified file.
     * If the user chooses to cancel, this function quits the closing process and
     * returns false. Files closed so far stay closed, the remaining open files stay
     * open.
     * If the user does not interrupt this function (e.g. no file was modified or
     * the user confirmed to save or to discard changes), all files get closed.
     * However, all files that were open before will be marked as opened
     * again. This could render the user interface inconsistent (therefore this
     * function should be called only when exiting KBibTeX), but it allows to
     * easily reopen in the next session all files that were open at the time
     * this function was called in this session.
     * @return true if all files could be closed successfully, else false.
     */
    bool queryCloseAll();

    void setCurrentFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr = KService::Ptr());
    OpenFileInfoList filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden = 0);

    friend class OpenFileInfo;

signals:
    void currentChanged(OpenFileInfo *, KService::Ptr);
    void flagsChanged(OpenFileInfo::StatusFlags statusFlags);

private:
    class OpenFileInfoManagerPrivate;
    OpenFileInfoManagerPrivate *d;

private slots:
    void deferredListsChanged();
    void delayedReadConfig();
};

#endif // KBIBTEX_PROGRAM_OPENFILEINFO_H
