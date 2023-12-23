/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect

Kirigami.ScrollablePage {
    id: root
    title: i18n("Plugin Settings")
    property string device

    ListView {
        anchors.fill: parent

        model: PluginModel {
            deviceId: device
        }

        delegate: Kirigami.SwipeListItem {

            contentItem: RowLayout {
                CheckBox {
                    id: serviceCheck
                    Layout.alignment: Qt.AlignVCenter
                    checked: model.isChecked
                    onToggled: model.isChecked = checked
                }

                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        Layout.fillWidth: true
                        text: model.name
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: model.description
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }
            }

            actions: [
                Kirigami.Action {
                    icon.name: "settings-configure"
                    visible: configSource != ""
                    onTriggered: {
                        if (pageStack.lastItem.toString().startsWith("PluginInfoPage")) {
                            pageStack.lastItem.configFile = configSource;
                            pageStack.lastItem.title = name;
                            pageStack.goForward();
                        } else {
                            pageStack.push(Qt.resolvedUrl("PluginInfoPage.qml"), {
                                title: name,
                                configFile: configSource,
                                device: root.device,
                            });
                        }
                    }
                }
            ]
        }
    }
}
