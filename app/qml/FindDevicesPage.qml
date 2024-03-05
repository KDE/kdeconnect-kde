/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect

Kirigami.ScrollablePage
{
    id: root

    Component {
        id: deviceComp
        DevicePage {}
    }

    objectName: "FindDevices"
    title: i18ndc("kdeconnect-app", "Title of the page listing the devices", "Devices")
    supportsRefreshing: true

    onRefreshingChanged: {
        DaemonDbusInterface.forceOnNetworkChange()
        refreshResetTimer.start()
    }

    Timer {
        id: refreshResetTimer
        interval: 1000
        onTriggered: root.refreshing = false
    }

    ListView {
        id: devices
        section {
            property: "status"
            delegate: Kirigami.ListSectionHeader {

                width: ListView.view.width

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
        Kirigami.PlaceholderMessage {
            text: i18nd("kdeconnect-app", "No devices found")
            anchors.centerIn: parent
            visible: devices.count === 0
        }

        model: DevicesSortProxyModel {
            sourceModel: DevicesModel {}
        }
        delegate: ItemDelegate {
            id: delegate
            icon.name: iconName
            icon.color: "transparent"
            text: model.name
            width: ListView.view.width
            highlighted: false

            contentItem: Kirigami.IconTitleSubtitle {
                title: delegate.text
                subtitle: toolTip
                icon: icon.fromControlsIcon(delegate.icon)
            }

            onClicked: {
                pageStack.push(
                    deviceComp,
                    {currentDevice: device}
                );
            }
        }
    }
}
