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

#ifndef KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H
#define KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"

/**
@author Thomas Fischer
*/
class KBIBTEXGUI_EXPORT SettingsFileExporterPDFPSWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsFileExporterPDFPSWidget(QWidget *parent);
    ~SettingsFileExporterPDFPSWidget();

    virtual QString label() const;
    virtual KIcon icon() const;

public slots:
    void loadState();
    void saveState();
    void resetToDefaults();

private:
    class SettingsFileExporterPDFPSWidgetPrivate;
    SettingsFileExporterPDFPSWidgetPrivate *d;
};

#endif // KBIBTEX_GUI_SETTINGSFILEEXPORTERPDFPSWIDGET_H
