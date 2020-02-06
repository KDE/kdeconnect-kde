/*
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.10 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage
{
    Component {
        id: deviceComp
        DevicePage {}
    }

    objectName: "FindDevices"
    title: i18nd("kdeconnect-app", "Pair")

    Label {
        text: i18nd("kdeconnect-app", "No devices found")
        anchors.centerIn: parent
        visible: devices.count === 0
    }

    ListView {
        id: devices
        section {
            property: "status"
            delegate: Kirigami.ListSectionHeader {
                text: switch (parseInt(section))
                {
                    case DevicesModel.Paired:
                        return i18nd("kdeconnect-app", "Remembered")
                    case DevicesModel.Reachable:
                        return i18nd("kdeconnect-app", "Available")
                    case (DevicesModel.Reachable | DevicesModel.Paired):
                        return i18nd("kdeconnect-app", "Connected")
                }
            }
        }

        model: DevicesSortProxyModel {
            sourceModel: DevicesModel {}
        }
        delegate: Kirigami.BasicListItem {
            width: ListView.view.width
            icon: iconName
            iconColor: "transparent"
            label: display + "\n" + toolTip
            highlighted: false
            onClicked: {
                pageStack.push(
                    deviceComp,
                    {currentDevice: device}
                );
            }
        }
    }
}
