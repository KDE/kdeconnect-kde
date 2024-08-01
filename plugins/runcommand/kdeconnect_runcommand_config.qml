/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

ListView {
    id: view
    Component.onCompleted: {
        root.leftPadding = 0
        root.rightPadding = 0
        root.topPadding = 0
        root.bottomPadding = 0
    }

    property string device

    property var action: Kirigami.Action {
        icon.name: "list-add"
        text: i18n("Add command")
        onTriggered: addDialog.open()
    }

    model: CommandsModel {
        id: commandModel
        deviceId: device
    }

    delegate: Kirigami.SwipeListItem {
        width: parent.width
        enabled: true

        contentItem: QQC2.Label {
            text: i18n("%1 <br> <i>%2</i>", name, command)
        }

        actions: Kirigami.Action {
            text: i18n("Delete")
            icon.name: "delete"
            onTriggered: commandModel.removeCommand(index)
        }
    }

    Kirigami.PlaceholderMessage {
        icon.name: 'utilities-terminal'
        anchors.centerIn: parent
        visible: view.count === 0
        width: parent.width - Kirigami.Units.gridUnit * 4
        text: i18n("No Commands")
        explanation: i18n("Add commands to run them remotely from other devices")
        helpfulAction: view.action
    }

    Kirigami.Dialog {
        id: addDialog
        title: "Add command"

        standardButtons: QQC2.Dialog.Save | QQC2.Dialog.Cancel
        padding: Kirigami.Units.largeSpacing

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
