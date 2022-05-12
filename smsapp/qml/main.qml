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

    property alias devicesCount : instantiator.count
    property QtObject device

    property var deviceActions : []

    Component {
        id: deviceActionComponent
        Kirigami.Action {
            required property string deviceId
            required property string name
            required property var device

            text: name

            onTriggered: {
                root.device = device
                AppData.initialDevice = ""
            }
            icon.name: root.device === device ? "checkmark" : ""
        }
    }

    Connections {
        target: AppData
        function onInitialDeviceChanged() {
            for (var action of root.deviceActions) {
                if (action.deviceId == AppData.initialDevice) {
                    root.device = action.device
                }
            }
        }
    }

    Instantiator {
        id: instantiator

        model: DevicesSortProxyModel {
            id: devicesModel
            //TODO: make it possible to filter if they can do sms
            sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
        }

        onObjectAdded: (idx, obj) => {
            root.deviceActions.push(obj)
            root.globalDrawer.actions[0].children = root.deviceActions

            if (!root.device && (AppData.initialDevice == "" || AppData.initialDevice === obj.deviceId)) {
                root.device = obj.device
            }
        }

        onObjectRemoved: (idx, obj) => {
            root.deviceActions.splice(idx, 1)
            root.globalDrawer.actions[0].children = root.deviceActions
        }

        delegate: deviceActionComponent
    }

    pageStack.initialPage: ConversationList {
        title: i18nd("kdeconnect-sms", "KDE Connect SMS")
        device: root.device;
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
