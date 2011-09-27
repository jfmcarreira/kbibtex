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

#include <QList>
#include <QPair>
#include <QLabel>
#include <QApplication>
#include <QSignalMapper>
#include <QLayout>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <KDebug>
#include <KPushButton>
#include <KIcon>
#include <KMessageBox>
#include <kmimetypetrader.h>
#include <kparts/part.h>
#include <kio/netaccess.h>

#include "mdiwidget.h"

const int URLRole = Qt::UserRole + 235;
const int SortRole = Qt::UserRole + 236;

class LRUItemModel : public QAbstractItemModel
{
private:
    OpenFileInfoManager *ofim;
public:
    LRUItemModel(QObject *parent = NULL)
            : QAbstractItemModel(parent), ofim(OpenFileInfoManager::getOpenFileInfoManager()) {
        // nothing
    }

    int columnCount(const QModelIndex &) const {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        QList<OpenFileInfo*> ofiList = ofim->filteredItems(OpenFileInfo::RecentlyUsed);
        if (index.row() < ofiList.count()) {
            OpenFileInfo *ofiItem = ofiList[index.row()];
            if (index.column() == 0) {
                if (role == Qt::DisplayRole || role == SortRole)
                    return ofiItem->url().fileName();
                else if (role == Qt::DecorationRole)
                    return KIcon(ofiItem->mimeType().replace("/", "-"));
                else if (role == Qt::ToolTipRole)
                    return ofiItem->url().pathOrUrl();
            } else if (index.column() == 1) {
                if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
                    return ofiItem->lastAccess().toString(Qt::TextDate);
                else if (role == SortRole)
                    return ofiItem->lastAccess().toTime_t();
            }
            if (role == URLRole)
                return ofiItem->url();
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation , int role = Qt::DisplayRole) const {
        if (role != Qt::DisplayRole || section >= 2)return QVariant();
        else if (section == 0) return i18n("Filename");
        else return i18n("Date/time of last use");
    }

    QModelIndex index(int row, int column, const QModelIndex &) const {
        return createIndex(row, column, row);
    }

    QModelIndex parent(const QModelIndex &) const {
        return QModelIndex();
    }

    int rowCount(const QModelIndex & parent = QModelIndex()) const {
        if (parent == QModelIndex())
            return ofim->filteredItems(OpenFileInfo::RecentlyUsed).count();
        else
            return 0;
    }
};

class MDIWidget::MDIWidgetPrivate
{
private:
    QTreeView *listLRU;
    LRUItemModel *modelLRU;

    void createWelcomeWidget() {
        welcomeWidget = new QWidget(p);
        QGridLayout *layout = new QGridLayout(welcomeWidget);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setRowStretch(2, 0);
        layout->setRowStretch(3, 0);
        layout->setRowStretch(4, 10);
        layout->setRowStretch(5, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 10);
        layout->setColumnStretch(2, 1);
        layout->setColumnStretch(3, 0);
        layout->setColumnStretch(4, 1);
        layout->setColumnStretch(5, 10);
        layout->setColumnStretch(6, 1);

        QLabel *label = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b></qt>"), p);
        layout->addWidget(label, 1, 2, 1, 3, Qt::AlignHCenter | Qt::AlignTop);

        KPushButton *buttonNew = new KPushButton(KIcon("document-new"), i18n("New"), p);
        layout->addWidget(buttonNew, 2, 2, 1, 1, Qt::AlignLeft | Qt::AlignBottom);
        connect(buttonNew, SIGNAL(clicked()), p, SIGNAL(documentNew()));

        KPushButton *buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open ..."), p);
        layout->addWidget(buttonOpen, 2, 4, 1, 1, Qt::AlignRight | Qt::AlignBottom);
        connect(buttonOpen, SIGNAL(clicked()), p, SIGNAL(documentOpen()));

        label = new QLabel(i18n("List of recently used files:"), p);
        layout->addWidget(label, 3, 1, 1, 5, Qt::AlignLeft | Qt::AlignBottom);
        listLRU = new QTreeView(p);
        listLRU->setRootIsDecorated(false);
        listLRU->setSortingEnabled(true);
        layout->addWidget(listLRU, 4, 1, 1, 5);
        connect(listLRU, SIGNAL(activated(QModelIndex)), p, SLOT(slotOpenLRU(QModelIndex)));
        label->setBuddy(listLRU);

        p->addWidget(welcomeWidget);
    }

public:
    MDIWidget *p;
    OpenFileInfo *currentFile;
    QWidget *welcomeWidget;
    QSignalMapper signalMapperCompleted;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent), currentFile(NULL) {
        createWelcomeWidget();

        modelLRU = new LRUItemModel(listLRU);
        QSortFilterProxyModel *sfpm = new QSortFilterProxyModel(listLRU);
        sfpm->setSourceModel(modelLRU);
        sfpm->setSortRole(SortRole);
        listLRU->setModel(sfpm);
        updateLRU();

        connect(&signalMapperCompleted, SIGNAL(mapped(QObject*)), p, SLOT(slotCompleted(QObject*)));
    }

    void addToMapper(OpenFileInfo *openFileInfo) {
        KParts::ReadOnlyPart *part = openFileInfo->part(p);
        signalMapperCompleted.setMapping(part, openFileInfo);
        connect(part, SIGNAL(completed()), &signalMapperCompleted, SLOT(map()));
    }

    void updateLRU() {

        listLRU->reset();
    }
};

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    connect(OpenFileInfoManager::getOpenFileInfoManager(), SIGNAL(flagsChanged(OpenFileInfo::StatusFlags)), this, SLOT(slotStatusFlagsChanged(OpenFileInfo::StatusFlags)));
}

void MDIWidget::setFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    BibTeXEditor *oldEditor = NULL;
    bool hasChanged = true;

    KParts::Part* part = openFileInfo == NULL ? NULL : openFileInfo->part(this, servicePtr);
    QWidget *widget = d->welcomeWidget;
    if (part != NULL) {
        widget = part->widget();
    } else if (openFileInfo != NULL) {
        KMessageBox::error(this, i18n("No part available for file '%1'.", openFileInfo->url().fileName()), i18n("No part available"));
        OpenFileInfoManager::getOpenFileInfoManager()->close(openFileInfo);
        return;
    }

    if (indexOf(widget) >= 0) {
        oldEditor = dynamic_cast<BibTeXEditor *>(currentWidget());
        hasChanged = widget != currentWidget();
    } else {
        addWidget(widget);
        d->addToMapper(openFileInfo);
    }
    setCurrentWidget(widget);
    d->currentFile = openFileInfo;

    if (hasChanged) {
        BibTeXEditor *newEditor = dynamic_cast<BibTeXEditor *>(widget);
        emit activePartChanged(part);
        emit documentSwitch(oldEditor, newEditor);
    }

    if (openFileInfo != NULL) {
        KUrl url = openFileInfo->url();
        if (url.isValid())
            emit setCaption(QString("%1 [%2]").arg(openFileInfo->shortCaption()).arg(openFileInfo->fullCaption()));
        else
            emit setCaption(openFileInfo->shortCaption());
    } else
        emit setCaption("");
}

BibTeXEditor *MDIWidget::editor()
{
    OpenFileInfo *ofi = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    return dynamic_cast<BibTeXEditor*>(ofi->part(this)->widget());
}

OpenFileInfo *MDIWidget::currentFile()
{
    return d->currentFile;
}

void MDIWidget::slotCompleted(QObject *obj)
{
    OpenFileInfo *ofi = static_cast<OpenFileInfo*>(obj);
    KUrl oldUrl = ofi->url();
    KUrl newUrl = ofi->part(this)->url();

    if (!oldUrl.equals(newUrl)) {
        kDebug() << "Url changed from " << oldUrl.pathOrUrl() << " to " << newUrl.pathOrUrl() << endl;
        OpenFileInfoManager::getOpenFileInfoManager()->changeUrl(ofi, newUrl);

        /// completely opened or saved files should be marked as "recently used"
        ofi->addFlags(OpenFileInfo::RecentlyUsed);

        emit setCaption(QString("%1 [%2]").arg(ofi->shortCaption()).arg(ofi->fullCaption()));
    }
}

void MDIWidget::slotStatusFlagsChanged(OpenFileInfo::StatusFlags statusFlags)
{
    if (statusFlags.testFlag(OpenFileInfo::RecentlyUsed))
        d->updateLRU();
}

void MDIWidget::slotOpenLRU(const QModelIndex&index)
{
    KUrl url = index.data(URLRole).value<KUrl>();
    if (url.isValid())
        emit documentOpenURL(url);
}
