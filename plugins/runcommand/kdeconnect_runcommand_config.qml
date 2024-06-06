/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage {
    id: root

    property string device

    actions: [
        Kirigami.Action {
            icon.name: "list-add"
            text: i18n("Add command")
            onTriggered: addDialog.open()
        }
    ]

    ListView {
        id: view

        model: CommandsModel {
            id: commandModel
            deviceId: device
        }

        delegate: QQC2.ItemDelegate {
            id: commandDelegate

            required property string name
            required property string command
            required property int index

            text: name
            width: ListView.view.width

            contentItem: RowLayout {
                Kirigami.TitleSubtitle {
                    title: commandDelegate.text
                    subtitle: commandDelegate.command
                    Layout.fillWidth: true
                }

                QQC2.ToolButton {
                    text: i18n("Delete")
                    icon.name: "delete"
                    onClicked: commandModel.removeCommand(commandDelegate.index)
                    display: QQC2.ToolButton.IconOnly

                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered || activeFocus
                }
            }
        }

        Kirigami.PlaceholderMessage {
            icon.name: 'utilities-terminal'
            anchors.centerIn: parent
            visible: view.count === 0
            width: parent.width - Kirigami.Units.gridUnit * 4
            text: i18n("No Commands")
            explanation: i18n("Add commands to run them remotely from other devices")
            helpfulAction: root.actions[0]
        }

        Kirigami.Dialog {
            id: addDialog
            title: "Add command"

            standardButtons: QQC2.Dialog.Save | QQC2.Dialog.Cancel

            horizontalPadding: Kirigami.Units.largeSpacing

            Kirigami.FormLayout {
                QQC2.TextField {
                    id: nameField
                    Kirigami.FormData.label: i18n("Name:")
                }
                QQC2.TextField {
                    id: commandField
                    Kirigami.FormData.label: i18n("Command:")
                }

                QQC2.ComboBox {
                    Kirigami.FormData.label: i18n("Sample commands:")
                    textRole: "name"
                    model: ListModel {
                        id: sampleCommands
                        ListElement {
                            name: "Sample command"
                            command: ""
                        }
                        ListElement {
                            name: "Suspend"
                            command: "systemctl suspend"
                        }
                        ListElement {
                            name: "Maximum Brightness"
                            command: "qdbus org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.setBrightness `qdbus org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.brightnessMax`"
                        }
                        ListElement {
                            name: "Lock Screen"
                            command: "loginctl lock-session"
                        }
                        ListElement {
                            name: "Unlock Screen"
                            command: "loginctl unlock-session"
                        }
                        ListElement {
                            name: "Close All Vaults"
                            command: "qdbus org.kde.kded5 /modules/plasmavault closeAllVaults"
                        }
                        ListElement {
                            name: "Forcefully Close All Vaults"
                            command: "qdbus org.kde.kded5 /modules/plasmavault forceCloseAllVaults"
                        }
                    }
                    onActivated: {
                        if (index > 0) {
                            nameField.text = sampleCommands.get(index).name
                            commandField.text = sampleCommands.get(index).command
                        }
                    }
                }
            }

            onAccepted: commandModel.addCommand(nameField.text, commandField.text)
        }
    }
}
