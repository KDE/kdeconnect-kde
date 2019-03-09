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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.people 1.0
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kdeconnect.sms 1.0

Kirigami.ScrollablePage
{
    id: page
    property alias personUri: person.personUri
    readonly property QtObject person: PersonData {
        id: person
    }

    property bool deviceConnected
    property string deviceId
//     property QtObject device
    property string conversationId
    property string initialMessage

    property string phoneNumber
    title: person.person && person.person.name ? person.person.name : phoneNumber

    Component.onCompleted: {
        if (initialMessage.length > 0) {
            message.text = initialMessage;
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
        messageBody: "Generic Message Body"
        dateTime: new Date('2000-0-0')
        visible: false
        enabled: false
    }

    function sendMessage() {
        model.sourceModel.sendReplyToConversation(message.text)
        message.text = ""
    }

    ListView {
        id: viewport
        model: QSortFilterProxyModel {
            id: model
            sortOrder: Qt.AscendingOrder
            sortRole: ConversationModel.DateRole
            sourceModel: ConversationModel {
                deviceId: page.deviceId
                threadId: page.conversationId
            }
        }

        spacing: Kirigami.Units.largeSpacing

        delegate: ChatMessage {
            messageBody: model.display
            sentByMe: model.fromMe
            dateTime: new Date(model.date)

            ListView.onAdd: {
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
                        viewport.currentIndex = index
                    }
            }
        }

        onMovementEnded: {
            // Unset the highlightRangeMode if it was set previously
            highlightRangeMode = ListView.ApplyRange
            highlightMoveDuration: -1 // "Re-enable" the highlight animation

            if (atYBeginning) {
                // "Lock" the view to the message currently at the beginning of the view
                // This prevents the view from snapping to the top of the messages we are about to request
                currentIndex = 0 // Index 0 is the beginning of the view
                preferredHighlightBegin = visibleArea.yPosition
                preferredHighlightEnd = preferredHighlightBegin + currentItem.height
                highlightRangeMode = ListView.StrictlyEnforceRange

                highlightMoveDuration = 1 // This is not ideal: I would like to disable the highlight animation altogether

                // Get more messages
                model.sourceModel.requestMoreMessages()
            }
        }
    }

    footer: RowLayout {
        enabled: page.deviceConnected
        ScrollView {
            Layout.maximumHeight: page.height / 3
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextArea {
                id: message
                Layout.fillWidth: true
                wrapMode: TextEdit.Wrap
                placeholderText: i18n("Say hi...")
                Keys.onPressed: {
                    if ((event.key == Qt.Key_Return) && !(event.modifiers & Qt.ShiftModifier)) {
                        sendMessage()
                        event.accepted = true
                    }
                }
            }
        }
        Button {
            text: "Send"
            onClicked: {
                sendMessage()
            }
        }
    }
}
