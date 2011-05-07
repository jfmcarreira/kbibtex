/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>

#include <KDebug>

#include "radiobuttontreeview.h"


RadioButtonItemDelegate::RadioButtonItemDelegate(QObject *p)
        : QStyledItemDelegate(p)
{
    // nothing
}

void RadioButtonItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (index.data(IsRadioRole).canConvert<bool>() && index.data(IsRadioRole).value<bool>()) {
        /// determine size and spacing of radio buttons in current style
        int radioButtonWidth = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
        int spacing = QApplication::style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &option);

        /// draw default appearance (text, highlighting) shifted to the left
        QStyleOptionViewItem myOption = option;
        int left = myOption.rect.left();
        myOption.rect.setLeft(left + spacing + radioButtonWidth);
        QStyledItemDelegate::paint(painter, myOption, index);

        /// draw radio button in the open space
        myOption.rect.setLeft(left);
        myOption.rect.setWidth(radioButtonWidth);
        if (index.data(RadioSelectedRole).canConvert<bool>()) {
            /// change radio button's visual appearance if selected or not
            bool radioButtonSelected = index.data(RadioSelectedRole).value<bool>();
            myOption.state |= radioButtonSelected ? QStyle::State_On : QStyle::State_Off;
        }
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &myOption, painter);
    } else
        QStyledItemDelegate::paint(painter, option, index);
}

QSize RadioButtonItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    if (index.parent() != QModelIndex()) {
        /// determine size of radio buttons in current style
        int radioButtonHeight = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
        /// ensure that line is tall enough to draw radio button
        s.setHeight(qMax(s.height(), radioButtonHeight));
    }
    return s;
}


RadioButtonTreeView::RadioButtonTreeView(QWidget *parent)
        : QTreeView(parent)
{
    // nothing
}

void RadioButtonTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index != QModelIndex() && index.parent() != QModelIndex()) {
        /// clicking on an alternative's item in tree view should select alternative
        switchRadioFlag(index);
        event->accept();
    } else
        QTreeView::mouseReleaseEvent(event);
}

void RadioButtonTreeView::keyReleaseEvent(QKeyEvent *event)
{
    QModelIndex index = currentIndex();
    if (index != QModelIndex() && index.parent() != QModelIndex() && event->key() == Qt::Key_Space) {
        /// pressing space on an alternative's item in tree view should select alternative
        switchRadioFlag(index);
        event->accept();
    } else
        QTreeView::keyReleaseEvent(event);
}

void RadioButtonTreeView::switchRadioFlag(QModelIndex &index)
{
    const int maxRow = 1024;
    const int col = index.column();
    for (int row = 0; row < maxRow; ++row) {
        const QModelIndex &sib = index.sibling(row, col);
        model()->setData(sib, QVariant::fromValue(sib == index), RadioSelectedRole);
    }
}