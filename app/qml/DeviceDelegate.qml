/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kdeconnect 1.0

Item
{
    height: info.height
    signal clicked
    QIconItem {
        id: icon
        width: 40
        height: parent.height
        icon: iconName
        anchors.verticalCenter: parent.verticalCenter
    }
    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
    ColumnLayout {
        id: info
        anchors {
            left: icon.right
            top: parent.top
            right: parent.right
        }
        property bool expand: false
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: display + "\n" + toolTip
        }
    }
}
