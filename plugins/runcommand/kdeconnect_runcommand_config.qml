/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage {
    id: root

    property string device

    actions: Kirigami.Action {
        id: addCommandDialogAction
        icon.name: "list-add"
        text: i18nd("kdeconnect-plugins", "Add Command…")
        onTriggered: addDialog.open()
    }

    ListView {
        id: view
        focus: view.count > 0

        model: CommandsModel {
            id: commandModel
            deviceId: device
        }

        delegate: Kirigami.SwipeListItem {
            width: parent.width
            enabled: true

            contentItem: ColumnLayout {
                QQC2.Label {
                    text: name
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                QQC2.Label {
                    text: command
                    font.italic: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            actions: [
                Kirigami.Action {
                    text: i18nd("kdeconnect-plugins", "Edit")
                    icon.name: "edit-entry"
                    onTriggered: {
                        editDialog.index = index;
                        editNameField.text = name;
                        editCommandField.text = command;
                        editDialog.open();
                    }
                },
                Kirigami.Action {
                    text: i18nd("kdeconnect-plugins", "Delete")
                    icon.name: "delete"
                    onTriggered: commandModel.removeCommand(index)
                }
            ]
        }

        Kirigami.PlaceholderMessage {
            icon.name: 'utilities-terminal'
            anchors.centerIn: parent
            visible: view.count === 0
            width: parent.width - Kirigami.Units.gridUnit * 4
            text: i18ndc("kdeconnect-plugins", "@info", "No commands configured")
            explanation: i18ndc("kdeconnect-plugins", "@info", "Add commands to run them remotely from other devices")
            helpfulAction: addCommandDialogAction
        }

        Kirigami.Dialog {
            id: addDialog
            title: i18ndc("kdeconnect-plugins", "@title:window", "Add Command")

            padding: Kirigami.Units.largeSpacing
            preferredWidth: Kirigami.Units.gridUnit * 20

            property Kirigami.Action addCommandAction: Kirigami.Action {
                text: i18ndc("kdeconnect-plugins", "@action:button", "Add")
                icon.name: "list-add"
                enabled: commandField.length > 0
                onTriggered: {
                    commandModel.addCommand(nameField.text, commandField.text);
                    addDialog.close();
                }
                Accessible.name: i18ndc("kdeconnect-plugins", "@action:button accessible", "Add command")
            }

            standardButtons: Kirigami.Dialog.Cancel
            customFooterActions: [addCommandAction]

            Kirigami.FormLayout {
                QQC2.TextField {
                    id: nameField
                    Kirigami.FormData.label: i18nd("kdeconnect-plugins", "Name:")
                }
                QQC2.TextField {
                    id: commandField
                    Kirigami.FormData.label: i18nd("kdeconnect-plugins", "Command:")
                }

                QQC2.ComboBox {
                    Kirigami.FormData.label: i18nd("kdeconnect-plugins", "Sample commands:")
                    visible: model.length > 0
                    textRole: "name"
                    currentIndex: -1
                    displayText: currentIndex === -1 ? "" : currentText
                    model: createSampleCommands()
                    onActivated: {
                        if (currentIndex >= 0) {
                            nameField.text = model[currentIndex].name;
                            commandField.text = model[currentIndex].command;
                        }
                    }
                    function createSampleCommands() {
                        // See https://doc.qt.io/qt-6/qml-qtqml-qt.html#platform-prop
                        // for possible platform values.
                        if (Qt.platform.os == "windows") {
                            return [
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Suspend"),
                                    command: "rundll32.exe powrprof.dll,SetSuspendState 0,1,0"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Lock Screen"),
                                    command: "rundll32.exe user32.dll,LockWorkStation"
                                }
                            ];
                        } else if (Qt.platform.os == "linux" || Qt.platform.os == "unix") {
                            return [
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Suspend"),
                                    command: "systemctl suspend"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Maximum Brightness"),
                                    command: "qdbus org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.setBrightness `qdbus org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.brightnessMax`"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Lock Screen"),
                                    command: "loginctl lock-session"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Unlock Screen"),
                                    command: "loginctl unlock-session"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Close All Vaults"),
                                    command: "qdbus org.kde.kded5 /modules/plasmavault closeAllVaults"
                                },
                                {
                                    name: i18ndc("kdeconnect-plugins", "Sample command", "Forcefully Close All Vaults"),
                                    command: "qdbus org.kde.kded5 /modules/plasmavault forceCloseAllVaults"
                                }
                            ];
                        } else {
                            return [];
                        }
                    }
                }
            }
        }
        Kirigami.Dialog {
            id: editDialog
            title: i18ndc("kdeconnect-plugins", "@title:window", "Edit Command")

            padding: Kirigami.Units.largeSpacing
            preferredWidth: Kirigami.Units.gridUnit * 20
            property int index: 0
            Kirigami.FormLayout {
                QQC2.TextField {
                    id: editNameField
                    Kirigami.FormData.label: i18nd("kdeconnect-plugins", "Name:")
                }
                QQC2.TextField {
                    id: editCommandField
                    Kirigami.FormData.label: i18nd("kdeconnect-plugins", "Command:")
                }
            }

            property Kirigami.Action editCommandAction: Kirigami.Action {
                text: i18ndc("kdeconnect-plugins", "@action:button", "Confirm")
                icon.name: "dialog-ok"
                enabled: editCommandField.length > 0
                onTriggered: {
                    commandModel.changeCommand(editDialog.index, editNameField.text, editCommandField.text);
                    editDialog.close();
                }
                Accessible.name: i18ndc("kdeconnect-plugins", "@action:button accessible", "Confirm")
            }

            standardButtons: Kirigami.Dialog.Cancel
            customFooterActions: [editCommandAction]
        }
    }
}
