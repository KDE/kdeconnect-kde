/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kdeconnect 1.0

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

    Kirigami.PlaceholderMessage {
        text: i18nd("kdeconnect-app", "No devices found")
        anchors.centerIn: parent
        visible: devices.count === 0
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
