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

#ifndef KBIBTEX_GUI_SETTINGSGLOBALKEYWORDSWIDGET_H
#define KBIBTEX_GUI_SETTINGSGLOBALKEYWORDSWIDGET_H

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"

class File;

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SettingsGlobalKeywordsWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsGlobalKeywordsWidget(QWidget *parent);
    ~SettingsGlobalKeywordsWidget();

    virtual QString label() const;
    virtual KIcon icon() const;

public slots:
    void loadState();
    void saveState();
    void resetToDefaults();

private slots:
    void addKeyword();
    void removeKeyword();
    void enableRemoveButton();

private:
    class SettingsGlobalKeywordsWidgetPrivate;
    SettingsGlobalKeywordsWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSGLOBALKEYWORDSWIDGET_H
