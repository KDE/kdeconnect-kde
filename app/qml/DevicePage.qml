/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtCore
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect

Kirigami.ScrollablePage {
    id: root
    property QtObject currentDevice
    title: currentDevice.name

    actions: [
        Kirigami.Action {
            icon.name: "network-disconnect"
            onTriggered: {
                root.currentDevice.unpair()
                pageStack.pop(0)
            }
            text: i18nd("kdeconnect-app", "Unpair")
            visible: root.currentDevice && root.currentDevice.isPaired
        },
        Kirigami.Action {
            icon.name: "hands-free"
            text: i18nd("kdeconnect-app", "Send Ping")
            visible: root.currentDevice && root.currentDevice.isPaired && root.currentDevice.isReachable
            onTriggered: {
                root.currentDevice.pluginCall("ping", "sendPing");
            }
        },
        Kirigami.Action {
            icon.name: "settings-configure"
            text: i18n("Plugin Settings")
            visible: root.currentDevice && root.currentDevice.isPaired && root.currentDevice.isReachable
            onTriggered: {
                pageStack.push(
                    Qt.resolvedUrl("PluginSettings.qml"),
                    {device: currentDevice.id()}
                );
            }
        }
    ]

    ListView {
        id: pluginsListView
        model: plugins

        section.property: "section"
        section.delegate: Kirigami.ListSectionHeader {
            width: ListView.view.width
            text: switch (section) {
                case "action":
                    return i18nc("@title:group device page section header", "Actions");
                case "control":
                    return i18nc("@title:group device page section header", "Controls");
            }
        }

        delegate: QQC2.ItemDelegate {
            id: pluginDelegate
            text: Kirigami.MnemonicData.richTextLabel
            Accessible.name: Kirigami.MnemonicData.plainTextLabel ?? modelData.name // fallback needed for KF < 6.12
            icon.name: modelData.iconName
            highlighted: false
            height: visible ? pluginDelegate.implicitHeight : -pluginsListView.spacing
            icon.color: "transparent"
            width: ListView.view.width
            onClicked: modelData.onClick()
            enabled: loaded
            visible: loaded

            Kirigami.MnemonicData.enabled: enabled && visible
            Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
            Kirigami.MnemonicData.label: modelData.name

            Shortcut {
                sequence: pluginDelegate.Kirigami.MnemonicData.sequence
                onActivated: clicked()
            }
        }

        property list<QtObject> plugins: [
            PluginItem {
                name: i18nd("kdeconnect-app", "Multimedia control")
                interfaceFactory: MprisDbusInterfaceFactory
                component: "mpris.qml"
                pluginName: "mprisremote"
                section: "control"
                device: root.currentDevice
            },
            PluginItem {
                name: i18nd("kdeconnect-app", "Remote input")
                interfaceFactory: RemoteControlDbusInterfaceFactory
                component: "mousepad.qml"
                pluginName: "remotecontrol"
                section: "control"
                device: root.currentDevice
            },
            PluginItem {
                name: i18nd("kdeconnect-app", "Run command")
                interfaceFactory: RemoteCommandsDbusInterfaceFactory
                component: "runcommand.qml"
                pluginName: "remotecommands"
                section: "control"
                device: root.currentDevice
            },
            PluginItem {
                name: i18nd("kdeconnect-app", "Presentation Remote")
                interfaceFactory: RemoteKeyboardDbusInterfaceFactory
                component: "presentationRemote.qml"
                pluginName: "remotecontrol"
                section: "control"
                device: root.currentDevice
            },
            PluginItem {
                name: i18nd("kdeconnect-app", "Volume control")
                interfaceFactory: RemoteSystemVolumeDbusInterfaceFactory
                component: "volume.qml"
                pluginName: "remotesystemvolume"
                section: "control"
                device: root.currentDevice
            },
            PluginItem {
                readonly property var lockIface: LockDeviceDbusInterfaceFactory.create(root.currentDevice.id())
                pluginName: "lockdevice"
                name: lockIface.isLocked ? i18nd("kdeconnect-app", "Unlock") : i18nd("kdeconnect-app", "Lock")
                onClick: () => lockIface.isLocked = !lockIface.isLocked;
                section: "action"
                device: root.currentDevice
            },
            PluginItem {
                readonly property var findmyphoneIface: FindMyPhoneDbusInterfaceFactory.create(root.currentDevice.id())
                pluginName: "findmyphone"
                name: i18nd("kdeconnect-app", "Find Device")
                section: "action"
                onClick: () => findmyphoneIface.ring()
                device: root.currentDevice
            },
            PluginItem {
                readonly property var clipboardIface: ClipboardDbusInterfaceFactory.create(root.currentDevice.id())
                pluginName: "clipboard"
                name: i18nd("kdeconnect-app", "Send Clipboard")
                onClick: () => clipboardIface.sendClipboard()
                section: "action"
                device: root.currentDevice
            },
            PluginItem {
                pluginName: "share"
                name: i18nd("kdeconnect-app", "Share File")
                onClick: () => fileDialog.open()
                section: "action"
                device: root.currentDevice
            }
        ]

        Kirigami.PlaceholderMessage {
            // FIXME: not accessible. screen readers won't read this. Idem for the other PlaceholderMessages
            //        https://invent.kde.org/frameworks/kirigami/-/merge_requests/1482
            text: i18nd("kdeconnect-app", "This device is not paired.")
            anchors.centerIn: parent
            visible: root.currentDevice && root.currentDevice.isReachable && !root.currentDevice.isPaired && !root.currentDevice.isPairRequestedByPeer && !root.currentDevice.isPairRequested
            helpfulAction: Kirigami.Action {
                text: i18ndc("kdeconnect-app", "Request pairing with a given device", "Pair")
                icon.name:"network-connect"
                onTriggered: root.currentDevice.requestPairing()
            }
        }

        Kirigami.PlaceholderMessage {
            text: i18nd("kdeconnect-app", "Pair requested\nKey: " + root.currentDevice.verificationKey)
            anchors.centerIn: parent
            visible: root.currentDevice && root.currentDevice.isReachable && root.currentDevice.isPairRequested
            QQC2.BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Kirigami.PlaceholderMessage {
            text: i18n("Pair requested\nKey: " + root.currentDevice.verificationKey)
            visible: root.currentDevice && root.currentDevice.isPairRequestedByPeer
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            RowLayout {
                QQC2.Button {
                    text: i18nd("kdeconnect-app", "Accept")
                    icon.name:"dialog-ok"
                    onClicked: root.currentDevice.acceptPairing()
                }

                QQC2.Button {
                    text: i18nd("kdeconnect-app", "Reject")
                    icon.name:"dialog-cancel"
                    onClicked: root.currentDevice.cancelPairing()
                }
            }
        }

        Kirigami.PlaceholderMessage {
            // FIXME: not accessible. screen readers won't read this.
            //        https://invent.kde.org/frameworks/kirigami/-/merge_requests/1482
            visible: root.currentDevice && !root.currentDevice.isReachable
            text: i18nd("kdeconnect-app", "This device is not reachable")
            anchors.centerIn: parent
        }
    }

    FileDialog {
        id: fileDialog
        readonly property var shareIface: root.currentDevice ? ShareDbusInterfaceFactory.create(root.currentDevice.id()) : null
        title: i18nd("kdeconnect-app", "Please choose a file")
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: shareIface.shareUrl(fileDialog.fileUrl)
    }
}
