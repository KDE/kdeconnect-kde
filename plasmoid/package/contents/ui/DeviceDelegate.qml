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
import org.kde.kdeconnect 1.0

PlasmaComponents.ListItem
{
    id: root
    property string deviceId: model.deviceId

    Column {
        width: parent.width
        PlasmaComponents.Label {
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            text: display
        }

        //Battery
        PlasmaComponents.ListItem {
            BatteryInterface {
                id: batteryInterface
                device: root.deviceId
            }
            sectionDelegate: true
            visible: batteryInterface.available
            PlasmaComponents.Label {
                //font.bold: true
                text: i18n("Battery")
            }
            PlasmaComponents.Label {
                text: batteryInterface.displayString
                anchors.right: parent.right
            }
        }

        //Notifications
        PlasmaComponents.ListItem {
            visible: notificationsModel.count>0
            enabled: true
            sectionDelegate: true
            PlasmaComponents.Label {
                //font.bold: true
                text: i18n("Notifications")
            }
            PlasmaComponents.ToolButton {
                enabled: true
                visible: notificationsModel.isAnyDimissable;
                anchors.right: parent.right
                iconSource: "window-close"
                onClicked: notificationsModel.dismissAll();
            }
        }
        Repeater {
            id: notificationsView
            model: NotificationsModel {
                id: notificationsModel
                deviceId: root.deviceId
            }
            delegate: PlasmaComponents.ListItem {
                PlasmaComponents.Label {
                    text: appName + ": " + display
                    anchors.right: dismissButton.left
                    anchors.left: parent.left
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
                PlasmaComponents.ToolButton {
                    id: dismissButton
                    visible: notificationsModel.isAnyDimissable;
                    enabled: dismissable
                    anchors.right: parent.right
                    iconSource: "window-close"
                    onClicked: dbusInterface.dismiss();
                }
            }
            //FIXME
            //Repeater.onItemAdded: plasmoid.status = "NeedsAttentionStatus";
        }

        //TODO: Other information could be displayed here

    }
}
