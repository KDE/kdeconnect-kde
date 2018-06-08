/*
 * Copyright 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page
{
    id: mousepad
    title: i18n("Presentation Remote")
    property QtObject pluginInterface

    actions.main: Kirigami.Action {
        icon.name: "view-fullscreen"
        text: i18n("Enable Full-Screen")
        onTriggered: {
            mousepad.pluginInterface.sendKeyPress("", 25 /*XK_F5*/);
        }
    }

    ColumnLayout
    {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-backward"
                onClicked: mousepad.pluginInterface.sendKeyPress("p");
            }
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-forward"
                onClicked: mousepad.pluginInterface.sendKeyPress("n");
            }
        }
    }
}
