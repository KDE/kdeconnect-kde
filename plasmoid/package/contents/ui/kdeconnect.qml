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
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.graphicswidgets 0.1 as PlasmaWidgets
import org.kde.kdeconnect 1.0 as KdeConnect

Item {
    id: kdeconnect

    property int minimumWidth: 290
    property int minimumHeight: 340

    //TODO: Show the full widget when it is not docked
    property Component compactRepresentation: CompactRepresentation { }

    PlasmaComponents.Label {
        visible: devicesView.count==0
        text: i18n("No paired devices available")
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
    }

    /*
    //Startup arguments
    PlasmaComponents.Label {
        visible: (startupArguments.length > 0)
        text: (""+startupArguments)
        anchors.fill: parent
    }
    */

    function shouldPlasmoidBeShown()
    {
        if (devicesView.count > 0) {
            plasmoid.status = ActiveStatus;
        } else {
            plasmoid.status = PassiveStatus;
        }
    }

    PlasmaExtras.ScrollArea {
        id: dialogItem
        anchors.fill: parent

        flickableItem: ListView {
            id: devicesView
            anchors.fill: parent
            model: KdeConnect.DevicesModel {
                id: connectDeviceModel
                displayFilter: StatusPaired | StatusReachable
            }
            delegate: DeviceDelegate { }
            onCountChanged: shouldPlasmoidBeShown()
            Component.onCompleted: shouldPlasmoidBeShown();
        }
    }

/*
    ListView {
        anchors.fill: parent
        id: devicesView
        model: KdeConnect.DevicesModel {
        displayFilter: 3
        }
        delegate: DeviceDelegate {}
    }
*/
}
