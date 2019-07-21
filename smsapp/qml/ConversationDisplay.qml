/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.people 1.0
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kdeconnect.sms 1.0
import QtGraphicalEffects 1.0

Kirigami.ScrollablePage
{
    id: page

    property bool deviceConnected
    property string deviceId
//     property QtObject device
    property string conversationId
    property bool isMultitarget
    property string initialMessage

    property bool isInitalized: false

    property var conversationModel: ConversationModel {
        deviceId: page.deviceId
        threadId: page.conversationId
    }

    property var addresses
    title: conversationModel.getTitleForAddresses(addresses)

    Component.onCompleted: {
        if (initialMessage.length > 0) {
            messageField.text = initialMessage;
            initialMessage = ""
        }
    }

    /**
     * Build a chat message which is representative of all chat messages
     *
     * In other words, one which I can use to get a reasonable height guess
     */
    ChatMessage {
        id: genericMessage
        senderName: "Generic Sender"
        messageBody: "Generic Message Body"
        dateTime: new Date('2000-0-0')
        visible: false
        enabled: false
    }

    ListView {
        id: viewport
        model: QSortFilterProxyModel {
            id: model
            sortOrder: Qt.AscendingOrder
            sortRole: ConversationModel.DateRole
            sourceModel: conversationModel
        }

        spacing: Kirigami.Units.largeSpacing
        highlightMoveDuration: 0

        delegate: ChatMessage {
            senderName: model.sender
            messageBody: model.display
            sentByMe: model.fromMe
            dateTime: new Date(model.date)

            ListView.onAdd: {
                if (!isInitalized) {
                    // If we aren't initalized, we need to request enough messages to fill the view
                    // In order to do that, request one more message until we have enough
                    viewport.forceLayout()

                    if (viewport.contentHeight < viewport.height) {
                        console.debug("Requesting another message to fill the screen")
                        conversationModel.requestMoreMessages(1)
                    } else {
                        // Finish intializing: Scroll to the bottom of the view
                        viewport.currentIndex = 0
                        isInitalized = true
                    }
                    return
                }

                if (index == viewport.count - 1)
                    // This message is being inserted at the newest position
                    // We want to scroll to show it if the user is "almost" looking at it

                    // Define some fudge area. If the message is being drawn offscreen but within
                    // this distance, we move to show it anyway.
                    // Selected to be genericMessage.height because that value scales for different
                    // font sizes / DPI / etc. -- Better ideas are welcome!
                    // Double the value works nicely
                    var offscreenFudge = 2 * genericMessage.height

                    var viewportYBottom = viewport.contentY + viewport.height

                    if (y < viewportYBottom + genericMessage.height) {
                        viewport.highlightMoveDuration = -1
                        viewport.currentIndex = index
                    }
            }
        }

        onMovementEnded: {
            if (!isInitalized) {
                return
            }
            // Unset the highlightRangeMode if it was set previously
            highlightRangeMode = ListView.ApplyRange
            highlightMoveDuration: -1 // "Re-enable" the highlight animation

            // If we have scrolled to the last message currently in the view, request some more
            if (atYBeginning) {
                // "Lock" the view to the message currently at the beginning of the view
                // This prevents the view from snapping to the top of the messages we are about to request
                currentIndex = 0 // Index 0 is the beginning of the view
                preferredHighlightBegin = visibleArea.yPosition
                preferredHighlightEnd = preferredHighlightBegin + currentItem.height
                highlightRangeMode = ListView.StrictlyEnforceRange

                highlightMoveDuration = 1 // This is not ideal: I would like to disable the highlight animation altogether

                // Get more messages
                conversationModel.requestMoreMessages()
            }
        }
    }

    footer: Pane {
        id: sendingArea
        enabled: page.deviceConnected && !page.isMultitarget
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

            TextArea {
                id: messageField
                Layout.fillWidth: true
                placeholderText: page.isMultitarget ? i18n("Replying to multitarget messages is not supported") : i18n("Compose message")
                wrapMode: TextArea.Wrap
                topPadding: Kirigami.Units.gridUnit * 0.5
                bottomPadding: topPadding
                selectByMouse: true
                background: Item {}
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

            ToolButton {
                id: sendButton
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
                onClicked: {
                    // don't send empty messages
                    if (!messageField.text.length) {
                        return
                    }

                    // disable the button to prevent sending
                    // the same message several times
                    sendButton.enabled = false

                    // send the message
                    conversationModel.sendReplyToConversation(messageField.text)
                    messageField.text = ""

                    // re-enable the button
                    sendButton.enabled = true
                }
            }
        }
    }
}
