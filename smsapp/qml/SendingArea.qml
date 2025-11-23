/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtCore
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import QtQuick.Dialogs
import org.kde.kdeconnect.sms

ColumnLayout {
    id: root
    property var addresses
    property var selectedFileUrls: []
    readonly property int maxMessageSize: 600000
    property alias text: messageField.text

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
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        fileMode: FileDialog.OpenFiles

        onAccepted: {
            root.selectedFileUrls = fileDialog.selectedFiles
            fileDialog.close()
        }
    }

    Controls.Pane {
        id: sendingArea
        enabled: page.deviceConnected
        implicitWidth: root.width
        padding: Kirigami.Units.gridUnit * 0.5
        wheelEnabled: true

        RowLayout {
            anchors.fill: parent

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.maximumHeight: page.height > 300 ? page.height / 3 : 2 * page.height / 3
                contentWidth: page.width - sendButtonArea.width
                clip: true
                Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff

                Controls.TextArea {
                    width: parent.width
                    id: messageField
                    placeholderText: i18nd("kdeconnect-sms", "Send a message…")
                    wrapMode: TextEdit.Wrap
                    topPadding: Kirigami.Units.gridUnit * 0.5
                    bottomPadding: topPadding
                    selectByMouse: true
                    hoverEnabled: true

                    Keys.onReturnPressed: event => {
                        if (event.key === Qt.Key_Return) {
                            if (event.modifiers & Qt.ShiftModifier) {
                                //remove any selected text and insert new line at cursor position
                                messageField.cursorSelection.text = ""
                                messageField.insert(messageField.cursorPosition,"\n")
                            } else {
                                sendButton.clicked()
                                event.accepted = true
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill : parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                        cursorShape: Qt.IBeamCursor
                        z: 1
                    }
                }
            }

            ColumnLayout {
                id: sendButtonArea
                Layout.alignment: Qt.AlignBottom

                RowLayout {
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
                            source: "mail-attachment-symbolic"
                            isMask: true
                            smooth: true
                            anchors.centerIn: parent
                            width: Kirigami.Units.gridUnit * 1.5
                            height: width
                        }

                        Controls.ToolTip.text: i18nc("@info:tooltip", "Attach an image or file")
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                        Controls.ToolTip.visible: hovered

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

                        Controls.ToolTip.text: i18nc("@info:tooltip", "Clear attachments")
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                        Controls.ToolTip.visible: hovered

                        onClicked: {
                            selectedFileUrls = []
                        }
                    }

                    Controls.ToolButton {
                        property bool isSendingInProcess: false

                        id: sendButton
                        enabled: (messageField.text.length || selectedFileUrls.length) && !isSendingInProcess
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

                        Controls.ToolTip.text: i18nc("@info:tooltip", "Send message")
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                        Controls.ToolTip.visible: hovered

                        onClicked: {

                            // prevent sending the same message several times
                            // and don't touch enabled property
                            if (isSendingInProcess){
                                return;
                            }
                            isSendingInProcess = true

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
                            }
                            isSendingInProcess = false
                        }
                    }
                }

                Controls.Label {
                    id: charCount
                    text: conversationModel.getCharCountInfo(messageField.text)
                    visible: text.length > 0
                    Layout.minimumWidth: Math.max(Layout.minimumWidth, width) // Make this label only grow, never shrink
                }
            }
        }

    }


}
