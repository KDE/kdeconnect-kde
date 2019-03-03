import QtQuick 2.2
import QtQuick.Controls 2.5
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kdeconnect 1.0

ListView {

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

            Label {
                text: i18n("%1 <br> <i>%2</i>", name, command)
            }

            actions: [
                Kirigami.Action {
                    icon.name: "delete"
                    onTriggered: commandModel.removeCommand(index)
                }
            ]
        }

    Dialog {
        id: addDialog
        title: "Add command"

        standardButtons: Dialog.Save | Dialog.Cancel

        Kirigami.FormLayout {
            TextField {
                id: nameField
                Kirigami.FormData.label: i18n("Name:")
            }
            TextField {
                id: commandField
                Kirigami.FormData.label: i18n("Command:")
            }

            ComboBox {
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
