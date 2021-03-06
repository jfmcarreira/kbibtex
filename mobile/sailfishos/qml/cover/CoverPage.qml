/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2016-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Column {
        anchors.centerIn: parent

        Image {
            id: logo
            source: 'qrc:/icons/kbibtex.svg'
            anchors.horizontalCenter: parent.horizontalCenter
            width: cover.width * 2 / 3
            height: sourceSize.height * width / sourceSize.width
        }

        Label {
            id: label
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("BibSearch")
        }
    }


    CoverActionList {
        CoverAction {
            iconSource: "image://theme/icon-cover-search"
            onTriggered: {
                pageStack.clear()
                pageStack.push(Qt.resolvedUrl("../pages/BibliographyListView.qml"))
                pageStack.push(Qt.resolvedUrl("../pages/SearchForm.qml"))
                mainWindow.activate()
            }
        }
    }
}
