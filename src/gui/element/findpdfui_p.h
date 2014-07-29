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

#ifndef KBIBTEX_GUI_FINDPDFUI_P_H
#define KBIBTEX_GUI_FINDPDFUI_P_H

#include <KWidgetItemDelegate>

#include "findpdf.h"

class QListView;

class FindPDF;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class PDFItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

private:
    QListView *m_parent;

public:
    PDFItemDelegate(QListView *itemView, QObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    /// get the list of widgets
    virtual QList<QWidget *> createItemWidgets() const;

    /// update the widgets
    virtual void updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const;

private slots:
    void slotViewPDF();
    void slotRadioNoDownloadToggled(bool);
    void slotRadioDownloadToggled(bool);
    void slotRadioURLonlyToggled(bool);
};


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class PDFListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent = NULL);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    QList<FindPDF::ResultItem> &m_resultList;
};


#endif // KBIBTEX_GUI_FINDPDFUI_P_H