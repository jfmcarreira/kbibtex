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

#ifndef KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H
#define KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"

class QTreeView;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SettingsColorLabelWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    SettingsColorLabelWidget(QWidget *parent);

public slots:
    void loadState();
    void saveState();
    void resetToDefaults();

private slots:
    void addColorDialog();
    void removeColor();
    void enableRemoveButton();

private:
    class SettingsColorLabelWidgetPrivate;
    SettingsColorLabelWidgetPrivate *d;
};


/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT ColorLabelContextMenu : public QObject
{
    Q_OBJECT

public:
    ColorLabelContextMenu(QTreeView *widget);

private slots:
    void colorActivated(const QString &colorString);

private:
    QTreeView *m_tv;
};

#endif // KBIBTEX_GUI_SETTINGSCOLORLABELWIDGET_H