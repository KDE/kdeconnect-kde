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
import org.kde.people
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates
import org.kde.kdeconnect
import org.kde.kdeconnect.sms

Kirigami.ScrollablePage
{
    id: page
    ToolTip {
        id: noDevicesWarning
        visible: !page.deviceConnected
        timeout: -1
        text: "⚠️ " + i18nd("kdeconnect-sms", "No devices available") + " ⚠️"

        MouseArea {
            // Detect mouseover and show another tooltip with more information
            anchors.fill: parent
            hoverEnabled: true

            ToolTip.visible: containsMouse
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            // TODO: Wrap text if line is too long for the screen
            ToolTip.text: i18nd("kdeconnect-sms", "No new messages can be sent or received, but you can browse cached content")
        }
    }

    ColumnLayout {
        id: loadingMessage
        visible: deviceConnected && view.count == 0 && currentSearchText.length == 0
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

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
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

    property int devicesCount

    readonly property bool deviceConnected: devicesCount > 0
    property string currentSearchText

    Component {
        id: chatView
        ConversationDisplay {
            deviceConnected: page.deviceConnected
        }
    }

    titleDelegate: RowLayout {
        id: headerLayout
        width: parent.width
        Keys.forwardTo: [filter]
        Kirigami.SearchField {
            /**
             * Used as the filter of the list of messages
             */
            id: filter
            placeholderText: i18nd("kdeconnect-sms", "Search or start conversation...")
            Layout.fillWidth: true
            Layout.fillHeight: true
            onTextChanged: {
                currentSearchText = filter.text;
                if (filter.text != "") {
                    view.model.setConversationsFilterRole(ConversationListModel.AddressesRole)
                } else {
                    view.model.setConversationsFilterRole(ConversationListModel.ConversationIdRole)
                }
                view.model.setFilterFixedString(SmsHelper.canonicalizePhoneNumber(filter.text))

                view.currentIndex = 0
                filter.forceActiveFocus();
            }
            Keys.onReturnPressed: {
                event.accepted = true
                view.currentItem.startChat()
            }
            Keys.onEscapePressed: {
                event.accepted = filter.text != ""
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
            text: i18nd("kdeconnect-sms", "New")
            visible: true
            enabled: SmsHelper.isAddressValid(filter.text) && deviceConnected
            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: i18nd("kdeconnect-sms", "Start new conversation")

            onClicked: {
                // We have to disable the filter temporarily in order to avoid getting key inputs accidently while processing the request
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
                onActivated: newButton.onClicked()
            }
        }
    }

    property alias conversationListModel: conversationListModel

    ListView {
        id: view
        currentIndex: 0

        model: QSortFilterProxyModel {
            sortOrder: Qt.DescendingOrder
            filterCaseSensitivity: Qt.CaseInsensitive
            sourceModel: ConversationListModel {
                id: conversationListModel
                deviceId: AppData.deviceId
            }
        }

        Keys.forwardTo: [headerItem]

        delegate: KirigamiDelegates.SubtitleDelegate
        {
            id: listItem
            icon.name: decoration
            text: displayNames
            subtitle: toolTip
            width: view.width

            property var thumbnail: attachmentPreview

            function startChat() {
                view.currentItem.forceActiveFocus();
                applicationWindow().pageStack.push(chatView, {
                                                       addresses: addresses,
                                                       conversationId: model.conversationId,
                                                       isMultitarget: isMultitarget,
                                                       initialMessage: page.initialMessage})
                initialMessage = ""
            }

            onClicked: {
                view.currentIndex = index
                startChat();
            }

            Kirigami.Icon {
                id: thumbnailItem
                source: {
                    if (!listItem.thumbnail) {
                        return undefined
                    }
                    if (listItem.thumbnail.hasOwnProperty) {
                        if (listItem.thumbnail.hasOwnProperty("name") && listItem.thumbnail.name !== "")
                            return listItem.thumbnail.name;
                        if (listItem.thumbnail.hasOwnProperty("source"))
                            return listItem.thumbnail.source;
                    }
                    return listItem.thumbnail;
                }
                property int size: Kirigami.Units.iconSizes.huge
                Layout.minimumHeight: size
                Layout.maximumHeight: size
                Layout.minimumWidth: size
                selected: (listItem.highlighted || listItem.checked || listItem.pressed)
                opacity: 1
                visible: source != undefined
            }

            // Keep the currently-open chat highlighted even if this element is not focused
            highlighted: ListView.isCurrentItem
        }

        Component.onCompleted: {
            currentIndex = -1
            focus = true
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: deviceConnected && view.count == 0 && currentSearchText.length != 0
            text: i18ndc("kdeconnect-sms", "Placeholder message text when no messages are found", "No matches")
        }
    }
}
