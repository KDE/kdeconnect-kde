/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.0
import org.kde.kirigami 2.7 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.Page
{
    id: deviceView
    property QtObject currentDevice
    title: currentDevice.name

    actions.contextualActions: deviceLoader.item.actions
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    Loader {
        id: deviceLoader
        anchors.fill: parent

        sourceComponent: {
            if (deviceView.currentDevice.hasPairingRequests) {
                return pairDevice;
            }

            if (deviceView.currentDevice.isReachable) {
                if (deviceView.currentDevice.isTrusted) {
                    return trustedDevice;
                } else {
                    return untrustedDevice;
                }
            } else {
                return notReachableDevice;
            }
        }

        Component {
            id: trustedDevice
            ListView {
                property list<QtObject> actions: [
                    Kirigami.Action {
                        iconName:"network-disconnect"
                        onTriggered: deviceView.currentDevice.unpair()
                        text: i18n("Unpair")
                    },
                    Kirigami.Action {
                        iconName:"hands-free"
                        text: i18n("Send Ping")
                        onTriggered: {
                            deviceView.currentDevice.pluginCall("ping", "sendPing");
                        }
                    }
                ]

                id: trustedView
                Layout.fillWidth: true

                model: plugins
                delegate: Kirigami.BasicListItem {
                    label: name
                    icon: iconName
                    iconColor: "transparent"
                    visible: loaded
                    onClicked: onClick()
                }

                property list<QtObject> plugins : [

                    PluginItem {
                        name: i18n("Multimedia control")
                        interfaceFactory: MprisDbusInterfaceFactory
                        component: "qrc:/qml/mpris.qml"
                        pluginName: "mprisremote"
                    },
                    PluginItem {
                        name: i18n("Remote input")
                        interfaceFactory: RemoteControlDbusInterfaceFactory
                        component: "qrc:/qml/mousepad.qml"
                        pluginName: "remotecontrol"
                    },
                    PluginItem {
                        name: i18n("Presentation Remote")
                        interfaceFactory: RemoteKeyboardDbusInterfaceFactory
                        component: "qrc:/qml/presentationRemote.qml"
                        pluginName: "remotecontrol"
                    },
                    PluginItem {
                        readonly property var lockIface: LockDeviceDbusInterfaceFactory.create(deviceView.currentDevice.id())
                        pluginName: "lockdevice"
                        name: lockIface.isLocked ? i18n("Unlock") : i18n("Lock")
                        onClick: function() {
                            lockIface.isLocked = !lockIface.isLocked;
                        }
                    },
                    PluginItem {
                        readonly property var findmyphoneIface: FindMyPhoneDbusInterfaceFactory.create(deviceView.currentDevice.id())
                        pluginName: "findmyphone"
                        name: i18n("Find Device")
                        onClick: function() {
                            findmyphoneIface.ring()
                        }
                    },
                    PluginItem {
                        name: i18n("Run command")
                        interfaceFactory: RemoteCommandsDbusInterfaceFactory
                        component: "qrc:/qml/runcommand.qml"
                        pluginName: "remotecommands"
                    },
                    PluginItem {
                        readonly property var shareIface: ShareDbusInterfaceFactory.create(deviceView.currentDevice.id())
                        pluginName: "share"
                        name: i18n("Share File")
                        onClick: function() {
                            fileDialog.open()
                            shareIface.shareUrl(fileDialog.fileUrl)
                        }
                    },
                    PluginItem {
                        name: i18n("Volume control")
                        interfaceFactory: RemoteSystemVolumeDbusInterfaceFactory
                        component: "qrc:/qml/volume.qml"
                        pluginName: "remotesystemvolume"
                    }
                ]
            }
        }
        Component {
            id: untrustedDevice
            Item {
                readonly property var actions: []
                Button {
                    anchors.centerIn: parent
                    text: i18n("Pair")
                    icon.name:"network-connect"
                    onClicked: deviceView.currentDevice.requestPair()
                }
            }
        }
        Component {
            id: pairDevice
            Item {
                readonly property var actions: []
                RowLayout {
                        anchors.centerIn: parent
                    Button {
                        text: i18n("Accept")
                        icon.name:"dialog-ok"
                        onClicked: deviceView.currentDevice.acceptPairing()
                    }

                    Button {
                        text: i18n("Reject")
                        icon.name:"dialog-cancel"                        
                        onClicked: deviceView.currentDevice.rejectPairing()
                    }
                }
            }
        }

        Component {
            id: notReachableDevice
            Item {
                property var actions: []

                Label {
                    anchors.centerIn: parent
                    text: i18n("This device is not reachable")
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: i18n("Please choose a file")
        folder: shortcuts.home
    }
}
