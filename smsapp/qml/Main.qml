/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect
import org.kde.kdeconnect.sms
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.config as KConfig

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 800
    height: 600

    property alias devicesCount : instantiator.count

    property var deviceActions : []

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    Component {
        id: deviceActionComponent
        Kirigami.Action {
            required property string deviceId
            required property string name

            text: name

            onTriggered: {
                AppData.deviceId = deviceId
            }
            icon.name: AppData.deviceId === deviceId ? "checkmark" : ""
        }
    }

    Instantiator {
        id: instantiator

        model: DevicesPluginFilterProxyModel {
            id: devicesModel
            pluginFilter: "kdeconnect_sms"
            sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
        }

        onObjectAdded: (idx, obj) => {
            root.deviceActions.push(obj)
            root.globalDrawer.actions[0].children = root.deviceActions

            if (!AppData.deviceId) {
                AppData.deviceId = obj.deviceId
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
        devicesCount: root.devicesCount;
    }

    Component {
        id: aboutPageComponent
        FormCard.AboutPage {}
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
                        applicationWindow().pageStack.layers.push(aboutPageComponent)
                    }
                }
            }
        ]
    }

}
