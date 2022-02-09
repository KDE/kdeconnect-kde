/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.kirigami 2.6 as Kirigami
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.sms 1.0

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 800
    height: 600

    property int currentDeviceIndex
    property int devicesCount
    property string initialDevice
    property QtObject device

    Component {
        id: deviceActionComponent
        Kirigami.Action {
            property int deviceIndex
            onTriggered: {
                root.currentDeviceIndex = deviceIndex
            }
            icon.name: root.currentDeviceIndex === deviceIndex ? "checkmark" : ""
        }
    }

    DevicesSortProxyModel {
        id: devicesModel
        //TODO: make it possible to filter if they can do sms
        sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
        function populateDevicesMenu() {
            root.globalDrawer.actions[0].children = [];
            for (var i = 0; i < devicesModel.rowCount(); i++) {
                var dev = devicesModel.data(devicesModel.index(i, 0), DevicesSortProxyModel.DisplayRole);
                var obj = deviceActionComponent.createObject(root.globalDrawer.actions[0], {
                    text: dev,
                    deviceIndex: i
                });
                root.globalDrawer.actions[0].children.push(obj);
            }
        }
        onRowsInserted: {
            if (root.currentDeviceIndex < 0) {
                if (root.initialDevice) {
                    root.currentDeviceIndex = devicesModel.rowForDevice(root.initialDevice);
                } else {
                    root.currentDeviceIndex = 0;
                }
            }
            root.device = root.currentDeviceIndex >= 0 ? devicesModel.data(devicesModel.index(root.currentDeviceIndex, 0), DevicesModel.DeviceRole) : null
            root.devicesCount = devicesModel.rowCount();
            populateDevicesMenu();
        }
        onRowsRemoved: {
            root.devicesCount = devicesModel.rowCount();
            populateDevicesMenu();
        }
    }
    onCurrentDeviceIndexChanged: {
        root.device = root.currentDeviceIndex >= 0 ? devicesModel.data(devicesModel.index(root.currentDeviceIndex, 0), DevicesModel.DeviceRole) : null
    }

    pageStack.initialPage: ConversationList {
        title: i18nd("kdeconnect-sms", "KDE Connect SMS")
        initialMessage: initialMessage
        device: root.device;
        initialDevice: initialDevice
        currentDeviceIndex: root.currentDeviceIndex;
        devicesCount: root.devicesCount;
    }

    Component {
        id: aboutPageComponent
        Kirigami.AboutPage {}
    }

    globalDrawer: Kirigami.GlobalDrawer {

        isMenu: true

        actions: [
            Kirigami.Action {
                text: i18nd("kdeconnect-sms", "Devices")
                icon.name: "phone"
                visible: devicesCount > 1
            },
            Kirigami.Action {
                text: i18nd("kdeconnect-sms", "Refresh")
                icon.name: "view-refresh"
                enabled: devicesCount > 0
                onTriggered: {
                    pageStack.initialPage.conversationListModel.refresh();
                }
            },
            Kirigami.Action {
                text: i18nd("kdeconnect-sms", "About")
                icon.name: "help-about"
                onTriggered: {
                    if (applicationWindow().pageStack.layers.depth < 2) {
                        applicationWindow().pageStack.layers.push(aboutPageComponent, { aboutData: aboutData })
                    }
                }
            }
        ]
    }

}
