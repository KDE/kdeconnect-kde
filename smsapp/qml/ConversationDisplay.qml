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
            sortOrder: Qt.DescendingOrder
            sortRole: ConversationModel.DateRole
            sourceModel: conversationModel
        }

        spacing: Kirigami.Units.largeSpacing
        highlightMoveDuration: 0
        verticalLayoutDirection: Qt.BottomToTop

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

            onMessageCopyRequested: {
                SmsHelper.copyToClipboard(message)
            }
        }
    }

    footer: SendingArea {
        id: sendingArea

        width: parent.width
        addresses: page.addresses
    }
}
