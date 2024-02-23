/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates
import org.kde.kdeconnect.sms

Kirigami.ScrollablePage
{
    id: page

    ToolTip {
        visible: !deviceConnected
        timeout: -1
        text: "⚠️ " + i18nd("kdeconnect-sms", "No devices available") + " ⚠️"

        MouseArea {
            // Detect mouseover and show another tooltip with more information
            anchors.fill: parent
            hoverEnabled: true

            ToolTip.visible: containsMouse
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: i18nd("kdeconnect-sms", "No new messages can be sent or received, but you can browse cached content")
        }
    }

    ColumnLayout {
        id: loadingMessage
        visible: deviceConnected && view.count === 0 && currentSearchText.length == 0
        anchors.centerIn: parent

        BusyIndicator {
            visible: loadingMessage.visible
            running: loadingMessage.visible
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }

        Label {
            text: i18nd("kdeconnect-sms", "Loading conversations from device. If this takes a long time, please wake up your device and then click Refresh.")
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.preferredWidth: page.width / 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Label {
            text: i18nd("kdeconnect-sms", "Tip: If you plug in your device, it should not go into doze mode and should load quickly.")
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.preferredWidth: page.width / 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Button {
            text: i18nd("kdeconnect-sms", "Refresh")
            icon.name: "view-refresh"
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            onClicked: {
                conversationListModel.refresh();
            }
        }
    }

    property string initialMessage : AppData.initialMessage
    property string currentSearchText
    property alias conversationListModel: conversationListModel
    property int devicesCount
    readonly property bool deviceConnected: devicesCount > 0

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
        position: Kirigami.InlineMessage.Header
        visible: page.initialMessage.length > 0
        text: i18nd("kdeconnect-sms", "Choose recipient")

        actions: [
          Kirigami.Action {
              icon.name: "dialog-cancel"
              text: i18nd("kdeconnect-sms", "Cancel")
              onTriggered: initialMessage = ""
            }
        ]
    }

    titleDelegate: RowLayout {
        spacing: Kirigami.Units.mediumSpacing

        Layout.fillWidth: true

        Keys.forwardTo: [filter]

        Kirigami.SearchField {
            /**
             * Used as the filter of the list of messages
             */
            id: filter
            Layout.fillWidth: true
            placeholderText: i18nd("kdeconnect-sms", "Search or start a conversation")
            onTextChanged: {
                currentSearchText = filter.text;
                if (filter.text !== "") {
                    view.model.setConversationsFilterRole(ConversationListModel.AddressesRole)
                } else {
                    view.model.setConversationsFilterRole(ConversationListModel.ConversationIdRole)
                }
                view.model.setFilterFixedString(SmsHelper.canonicalizePhoneNumber(filter.text))

                view.currentIndex = 0
                filter.forceActiveFocus();
            }

            Keys.onReturnPressed: event => {
                event.accepted = true
                view.currentItem.startChat()
            }
            Keys.onEscapePressed: event => {
                event.accepted = filter.text !== ""
                filter.text = ""
            }
            Shortcut {
                sequence: "Ctrl+F"
                onActivated: filter.forceActiveFocus()
            }
        }

        Button {
            id: newButton
            icon.name: "list-add"
            enabled: SmsHelper.isAddressValid(filter.text) && deviceConnected

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: i18nd("kdeconnect-sms", "Start new conversation")

            onClicked: {
                // We have to disable the filter temporarily in order to avoid getting key inputs accidentally while processing the request
                filter.enabled = false

                // If the address entered by the user already exists then ignore adding new contact
                if (!view.model.doesAddressExists(filter.text) && SmsHelper.isAddressValid(filter.text)) {
                    conversationListModel.createConversationForAddress(filter.text)
                    view.currentIndex = 0
                }
                filter.enabled = true
            }

            Shortcut {
                sequence: "Ctrl+N"
                onActivated: newButton.clicked()
            }
        }
    }

    Component {
        id: chatView
        ConversationDisplay {
            deviceConnected: page.deviceConnected
        }
    }

    ListView {
        id: view
        model: QSortFilterProxyModel {
            sortOrder: Qt.DescendingOrder
            filterCaseSensitivity: Qt.CaseInsensitive
            sourceModel: ConversationListModel {
                id: conversationListModel
                deviceId: AppData.deviceId
            }
        }

        Component.onCompleted: {
            currentIndex = -1
            focus = true
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: deviceConnected && view.count === 0 && currentSearchText.length !== 0
            icon.name: "search"
            text: i18ndc("kdeconnect-sms", "Placeholder message text when no messages are found", "No Matches")
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)

            visible: view.count === 0 && currentSearchText.length === 0

            icon.name: "view-conversation-balloon"
            text: i18ndc("kdeconnect-sms", "@info:placeholder", "No Conversations")
        }

        Keys.forwardTo: [headerItem]

        delegate: ItemDelegate {
            id: listItem
            text: displayNames
            width: view.width

            required property string displayNames
            required property string toolTip
            required property var decoration
            required property var attachmentPreview
            required property int index
            required property var addresses
            required property bool isMultitarget
            required property int conversationId

            // Keep the currently-open chat highlighted even if this element is not focused
            highlighted: ListView.isCurrentItem

            function startChat() {
                view.currentItem.forceActiveFocus();
                applicationWindow().pageStack.push(chatView, {
                                                       addresses: listItem.addresses,
                                                       conversationId: listItem.conversationId,
                                                       isMultitarget: listItem.isMultitarget,
                                                       initialMessage: page.initialMessage})
                initialMessage = ""
            }

            onClicked: {
                view.currentIndex = index
                startChat();
            }

            // Note: Width calcs to account for scrollbar coming and going
            contentItem: RowLayout {
                id: contentRow
                spacing: Kirigami.Units.smallSpacing
                implicitWidth: view.width - Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    id: contactIcon
                    source: listItem.decoration
                }

                // Set width here to force elide and account for scrollbar
                KirigamiDelegates.TitleSubtitle {
                    title: listItem.text
                    subtitle: listItem.toolTip
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Kirigami.Icon {
                    id: thumbnailItem
                    source: {
                        if (!listItem.attachmentPreview) {
                            return undefined
                        }

                        if (listItem.attachmentPreview.hasOwnProperty("name") && listItem.attachmentPreview.name !== "")
                            return listItem.attachmentPreview.name;
                        if (listItem.attachmentPreview.hasOwnProperty("source"))
                            return listItem.attachmentPreview.source;
                        return listItem.attachmentPreview;
                    }

                    visible: source !== undefined
                }
            }
        }
    }
}
