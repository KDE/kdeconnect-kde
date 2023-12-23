/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.people
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect.sms
import Qt5Compat.GraphicalEffects

Kirigami.ScrollablePage
{
    id: page

    property bool deviceConnected
    property string conversationId
    property bool isMultitarget
    property string initialMessage
    property string invalidId: "-1"

    property bool isInitalized: false

    property var conversationModel: ConversationModel {
        deviceId: AppData.deviceId
        threadId: page.conversationId
        addressList: page.addresses

        onLoadingFinished: {
            page.isInitalized = true
        }
    }

    property var addresses
    title: SmsHelper.getTitleForAddresses(addresses)

    Component.onCompleted: {
        if (initialMessage.length > 0) {
            sendingArea.text = initialMessage;
            initialMessage = ""
        }
        if (conversationId == invalidId) {
            isInitalized = true
        }
    }

    /**
     * Build a chat message which is representative of all chat messages
     *
     * In other words, one which I can use to get a reasonable height guess
     */
    ChatMessage {
        id: genericMessage
        name: "Generic Sender"
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

        Controls.BusyIndicator {
            running: !isInitalized
            anchors.centerIn: parent
        }

        header: Item {
            height: Kirigami.Units.largeSpacing * 2
        }
        headerPositioning: ListView.InlineHeader

        onContentHeightChanged: {
            if (viewport.contentHeight <= 0) {
                return
            }

            if (!isInitalized) {
                // If we aren't initialized, we need to request enough messages to fill the view
                // In order to do that, request one more message until we have enough
                if (viewport.contentHeight < viewport.height) {
                    console.debug("Requesting another message to fill the screen")
                    conversationModel.requestMoreMessages(1)
                } else {
                    // Finish initializing: Scroll to the bottom of the view

                    // View the most-recent message
                    viewport.forceLayout()
                    Qt.callLater(viewport.positionViewAtEnd)
                    isInitalized = true
                }
                return
            }
        }

        delegate: ChatMessage {
            name: model.sender
            messageBody: model.display
            sentByMe: model.fromMe
            dateTime: new Date(model.date)
            multiTarget: isMultitarget
            attachmentList: model.attachments

            width: viewport.width

            ListView.onAdd: {
                if (!isInitalized) {
                    return
                }

                if (index == viewport.count - 1) {
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

            onMessageCopyRequested: {
                SmsHelper.copyToClipboard(message)
            }
        }

        /// As the user scrolls, load more messages when they get to the top.
        /// This used to use the onMovementEnded signal, but at some point
        /// that signal stopped being emitted reliably when scrolling with
        /// the mouse. onContentYChanged is fine for our use, just a bit noisy.
        onContentYChanged: {
            if (!isInitalized) {
                return
            }

            // If we have scrolled near to the top, request more messages
            // This threshold of visibleArea.yPosition has been chosen experimentally, but
            // should generally be OK because it is defined as a ratio of the content to the visible
            // area. As more messages get loaded into our view, this constant will start to be
            // less-sane, meaning we will request messages earlier as the user scrolls back in time.
            // This is probably a good thing, because it means that scrolling back further and further
            // quickly, will be more likely to be smooth.
            // Combined with `atYBeginning`, the view scrolls smoothly for me, long past the point where
            // the rest of the app is stable, and past the point where Android can fetch messages fast
            // enough.
            if (visibleArea.yPosition < 0.075 || atYBeginning) {
                // "Lock" the view to the message currently at the beginning of the view
                // This prevents the view from snapping to the top of the messages we are about to request
                currentIndex = 0 // Index 0 is the beginning of the view
                preferredHighlightBegin = visibleArea.yPosition
                preferredHighlightEnd = preferredHighlightBegin + currentItem.height
                highlightRangeMode = ListView.StrictlyEnforceRange

                highlightMoveDuration = 0

                // Get more messages
                conversationModel.requestMoreMessages()
            }
        }
    }

    footer: SendingArea {
        id: sendingArea

        width: parent.width
        addresses: page.addresses
    }
}
