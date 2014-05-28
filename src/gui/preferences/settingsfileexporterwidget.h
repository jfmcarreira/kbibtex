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

#ifndef KBIBTEX_GUI_SETTINGSFILEEXPORTERWIDGET_H
#define KBIBTEX_GUI_SETTINGSFILEEXPORTERWIDGET_H

#include <kbibtexgui_export.h>

#include "settingsabstractwidget.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT SettingsFileExporterWidget : public SettingsAbstractWidget
{
    Q_OBJECT

public:
    explicit SettingsFileExporterWidget(QWidget *parent);
    ~SettingsFileExporterWidget();

    virtual QString label() const;
    virtual KIcon icon() const;

public slots:
    void loadState();
    void saveState();
    void resetToDefaults();
    void automaticLyXDetectionToggled(bool);

private slots:
    void updateGUI();

private:
    class SettingsFileExporterWidgetPrivate;
    SettingsFileExporterWidgetPrivate *d;
};


#endif // KBIBTEX_GUI_SETTINGSFILEEXPORTERWIDGET_H
