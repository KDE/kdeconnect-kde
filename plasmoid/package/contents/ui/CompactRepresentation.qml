/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore

Item {
    id: view
    anchors.fill: parent

    //TODO: Use this to detect if we should be iconized or full size
    function isConstrained() {
        return (plasmoid.formFactor == Vertical || plasmoid.formFactor == Horizontal);
    }

    PlasmaCore.IconItem {
        id: icon
        source: "kdeconnect"
        anchors.fill: parent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: plasmoid.togglePopup()

        PlasmaCore.ToolTip {
            id: tooltip
            target: mouseArea
            image: QIcon("kdeconnect")
            subText: i18n("KDE Connect device notifications")
        }
    }

}
