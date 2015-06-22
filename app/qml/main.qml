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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kdeconnect 1.0

ApplicationWindow
{
    id: root
    visible: true
    width: 400
    height: 500

    toolBar: RowLayout {
        Button {
            text: "<"
            onClicked: stack.pop()
        }
        Label {
            Layout.fillWidth: true
            text: "KDE Connect"
            font.pointSize: 20
        }
    }
    StackView {
        id: stack
        anchors {
            fill: parent
            margins: 5
        }
        initialItem: ScrollView {
            Layout.fillHeight: true
            ListView {
                id: devicesView

                section {
                    property: "status"
                    delegate: Label {
                        text: switch (parseInt(section))
                        {
                            case DevicesModel.Paired:
                                return i18n("Paired")
                            case DevicesModel.Reachable:
                                return i18n("Reachable")
                            case (DevicesModel.Reachable | DevicesModel.Paired):
                                return i18n("Paired & Reachable")
                        }

                    }
                }

                spacing: 5
                model: DevicesSortProxyModel {
                    sourceModel: DevicesModel {
                        id: connectDeviceModel
                        displayFilter: DevicesModel.Reachable
                    }
                }
                delegate: DeviceDelegate {
                    width: parent.width
                    onClicked: {
                        var data = {
                            item: deviceViewComponent,
                            properties: {currentDevice: device}
                        };
                        stack.push(data);
                    }
                }
            }
        }
    }

    Component {
        id: deviceViewComponent
        ColumnLayout {
            id: deviceView
            property QtObject currentDevice
            Loader {
                Layout.fillHeight: true
                Layout.fillWidth: true

                sourceComponent: currentDevice.isPaired ? trustedDevice : untrustedDevice
                Component {
                    id: trustedDevice
                    ColumnLayout {
                        id: trustedView
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        Button {
                            text: i18n("Un-Pair")
                            onClicked: deviceView.currentDevice.unpair()
                        }
                        Button {
                            text: i18n("Send Ping")
                            onClicked: deviceView.currentDevice.pluginCall("ping", "sendPing");
                        }
                        Button {
                            text: i18n("Open Multimedia Remote Control")
                            onClicked: stack.push( {
                                item: "qrc:/qml/mpris.qml",
                                properties: { mprisInterface: MprisDbusInterfaceFactory.create(deviceView.currentDevice.id) }
                            } );
                        }
                        Button {
                            text: i18n("Remote touch and keyboard")
                            enabled: false
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
                Component {
                    id: untrustedDevice
                    ColumnLayout {
                        id: untrustedView
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        Button {
                            text: i18n("Pair")
                            onClicked: deviceView.currentDevice.requestPair()
                        }
                    }
                }
            }
        }
    }
}
