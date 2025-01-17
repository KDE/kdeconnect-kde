/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.kdeconnect

Kirigami.ScrollablePage {
    id: root

    property string device
    property string filterString

    title: i18n("Plugin Settings")

    header: Control {
        topPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing
        rightPadding: Kirigami.Units.smallSpacing

        background: Rectangle {
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.inherit: false
            color: Kirigami.Theme.backgroundColor

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                }
            }
        }

        contentItem: Kirigami.SearchField {
            id: searchField
            onTextChanged: root.filterString = text;
            autoAccept: false
            focus: true
        }
    }

    ListView {
        Accessible.role: Accessible.List
        model: KItemModels.KSortFilterProxyModel {
            filterString: root.filterString
            filterRoleName: "name"
            filterCaseSensitivity: Qt.CaseInsensitive

            sourceModel: PluginModel {
                deviceId: device
            }
        }

        delegate: Kirigami.SwipeListItem {

            contentItem: RowLayout {
                CheckBox {
                    id: serviceCheck
                    Layout.alignment: Qt.AlignVCenter
                    checked: model.isChecked
                    onToggled: model.isChecked = checked
                    Accessible.name: model.name
                    Accessible.description: model.description
                }

                Kirigami.Icon {
                    source: model.iconName
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
                    // FIXME: not accessible. screen readers won't read this and just say "push button".
                    //        https://bugreports.qt.io/browse/QTBUG-123123
                    Accessible.name: i18nd("kdeconnect-app", "Configure plugin")
                    onTriggered: {
                        pageStack.push(configSource, {
                            title: name,
                            device: root.device,
                        });
                    }
                }
            ]
        }
    }
}
