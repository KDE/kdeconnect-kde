/**
 * Copyright (C) 2020 Aniket Kumar <anikketkumar786@gmail.com>
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

import QtQuick 2.1
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.4 as Kirigami
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.1
import org.kde.kdeconnect.sms 1.0

ColumnLayout {
    id: root
    property var addresses
    property var selectedFileUrls: []
    readonly property int maxMessageSize: 600000

    MessageDialog {
        id: messageDialog
        title: i18nd("kdeconnect-sms", "Failed to send")
        text: i18nd("kdeconnect-sms", "Max message size limit exceeded.")

        onAccepted: {
            messageDialog.close()
        }
    }

    FileDialog {
        id: fileDialog
        folder: shortcuts.home
        selectMultiple: true

        onAccepted: {
            root.selectedFileUrls = fileDialog.fileUrls
            fileDialog.close()
        }
    }

    Controls.Pane {
        id: sendingArea
        enabled: page.deviceConnected
        layer.enabled: sendingArea.enabled
        layer.effect: DropShadow {
            verticalOffset: 1
            color: Kirigami.Theme.disabledTextColor
            samples: 20
            spread: 0.3
        }
        Layout.fillWidth: true
        padding: 0
        wheelEnabled: true
        background: Rectangle {
            color: Kirigami.Theme.viewBackgroundColor
        }

        RowLayout {
            anchors.fill: parent

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: page.height > 300 ? page.height / 3 : 2 * page.height / 3
                contentWidth: page.width - sendButtonArea.width
                clip: true
                Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff

                Controls.TextArea {
                    anchors.fill: parent
                    id: messageField
                    placeholderText: i18nd("kdeconnect-sms", "Compose message")
                    wrapMode: TextEdit.Wrap
                    topPadding: Kirigami.Units.gridUnit * 0.5
                    bottomPadding: topPadding
                    selectByMouse: true
                    topInset: height * 2 // This removes background (frame) of the TextArea. Setting `background: Item {}` would cause segfault.
                    Keys.onReturnPressed: {
                        if (event.key === Qt.Key_Return) {
                            if (event.modifiers & Qt.ShiftModifier) {
                                messageField.append("")
                            } else {
                                sendButton.onClicked()
                                event.accepted = true
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                id: sendButtonArea

                RowLayout {
                    Controls.ToolButton {
                        id: sendButton
                        enabled: messageField.text.length || selectedFileUrls.length
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                        padding: 0
                        Kirigami.Icon {
                            source: "document-send"
                            enabled: sendButton.enabled
                            isMask: true
                            smooth: true
                            anchors.centerIn: parent
                            width: Kirigami.Units.gridUnit * 1.5
                            height: width
                        }

                        property bool messageSent: false

                        onClicked: {
                            // disable the button to prevent sending
                            // the same message several times
                            sendButton.enabled = false

                            if (SmsHelper.totalMessageSize(selectedFileUrls, messageField.text) > maxMessageSize) {
                                messageDialog.visible = true
                            } else if (page.conversationId === page.invalidId) {
                                messageSent = conversationModel.startNewConversation(messageField.text, addresses, selectedFileUrls)
                            } else {
                                messageSent = conversationModel.sendReplyToConversation(messageField.text, selectedFileUrls)
                            }

                            if (messageSent) {
                                messageField.text = ""
                                selectedFileUrls = []
                                sendButton.enabled = false
                            }
                        }
                    }

                    Controls.ToolButton {
                        id: attachFilesButton
                        enabled: true
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                        padding: 0
                        Text {
                            id: attachedFilesCount
                            text: selectedFileUrls.length
                            color: "red"
                            visible: selectedFileUrls.length > 0
                        }
                        Kirigami.Icon {
                            source: "insert-image"
                            isMask: true
                            smooth: true
                            anchors.centerIn: parent
                            width: Kirigami.Units.gridUnit * 1.5
                            height: width
                        }

                        onClicked: {
                            fileDialog.open()
                        }
                    }

                    Controls.ToolButton {
                        id: clearAttachmentButton
                        visible: selectedFileUrls.length > 0
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                        padding: 0
                        Kirigami.Icon {
                            id: cancelIcon
                            source: "edit-clear"
                            isMask: true
                            smooth: true
                            anchors.centerIn: parent
                            width: Kirigami.Units.gridUnit * 1.5
                            height: width
                        }

                        onClicked: {
                            selectedFileUrls = []
                            if (messageField.text == "") {
                                sendButton.enabled = false
                            }
                        }
                    }
                }

                Controls.Label {
                    id: "charCount"
                    text: conversationModel.getCharCountInfo(messageField.text)
                    visible: text.length > 0
                    Layout.minimumWidth: Math.max(Layout.minimumWidth, width) // Make this label only grow, never shrink
                }
            }
        }
    }
}
