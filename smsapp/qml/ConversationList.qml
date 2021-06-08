/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.people 1.0
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.sms 1.0

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

    contextualActions: [
        Kirigami.Action {
            text: i18nd("kdeconnect-sms", "Refresh")
            icon.name: "view-refresh"
            enabled: devicesCombo.count > 0
            onTriggered: {
                conversationListModel.refresh()
            }
        }
    ]

    ColumnLayout {
        id: loadingMessage
        visible: deviceConnected && view.count == 0 && view.headerItem.childAt(0, 0).text.length == 0
        anchors.centerIn: parent

        BusyIndicator {
            running: loadingMessage.visible
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }

        Label {
            text: "Loading conversations from device. If this takes a long time, please wake up your device and then click refresh."
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.preferredWidth: page.width / 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Label {
            text: "Tip: If you plug in your device, it should not go into doze mode and should load quickly."
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.preferredWidth: page.width / 2
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    property string initialMessage
    property string initialDevice

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: page.initialMessage.length > 0
        text: i18nd("kdeconnect-sms", "Choose recipient")

        actions: [
          Kirigami.Action {
              iconName: "dialog-cancel"
              text: i18nd("kdeconnect-sms", "Cancel")
              onTriggered: initialMessage = ""
            }
        ]
    }

    footer: ComboBox {
        id: devicesCombo
        enabled: count > 0
        displayText: !enabled ? i18nd("kdeconnect-sms", "No devices available") : undefined
        model: DevicesSortProxyModel {
            id: devicesModel
            //TODO: make it possible to filter if they can do sms
            sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
            onRowsInserted: if (devicesCombo.currentIndex < 0) {
                if (page.initialDevice)
                    devicesCombo.currentIndex = devicesModel.rowForDevice(page.initialDevice);
                else
                    devicesCombo.currentIndex = 0
            }
        }
        textRole: "display"
    }

    readonly property bool deviceConnected: devicesCombo.enabled
    readonly property QtObject device: devicesCombo.currentIndex >= 0 ? devicesModel.data(devicesModel.index(devicesCombo.currentIndex, 0), DevicesModel.DeviceRole) : null
    readonly property alias lastDeviceId: conversationListModel.deviceId

    Component {
        id: chatView
        ConversationDisplay {
            deviceId: page.lastDeviceId
            deviceConnected: page.deviceConnected
        }
    }

    ListView {
        id: view
        currentIndex: 0

        model: QSortFilterProxyModel {
            sortOrder: Qt.DescendingOrder
            filterCaseSensitivity: Qt.CaseInsensitive
            sourceModel: ConversationListModel {
                id: conversationListModel
                deviceId: device ? device.id() : ""
            }
        }

        header: RowLayout {
            width: parent.width
            z: 10
            Keys.forwardTo: [filter]
            TextField {
                /**
                 * Used as the filter of the list of messages
                 */
                id: filter
                placeholderText: i18nd("kdeconnect-sms", "Filter...")
                Layout.fillWidth: true
                Layout.fillHeight: true
                onTextChanged: {
                    if (filter.text != "") {
                        view.model.setConversationsFilterRole(ConversationListModel.AddressesRole)
                    } else {
                        view.model.setConversationsFilterRole(ConversationListModel.ConversationIdRole)
                    }
                    view.model.setFilterFixedString(SmsHelper.canonicalizePhoneNumber(filter.text))

                    view.currentIndex = 0
                }
                onAccepted: {
                    view.currentItem.startChat()
                }
                Keys.onReturnPressed: {
                    event.accepted = true
                    filter.onAccepted()
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

        headerPositioning: ListView.OverlayHeader

        Keys.forwardTo: [headerItem]

        delegate: Kirigami.BasicListItem
        {
            id: listItem
            icon: decoration
            reserveSpaceForIcon: true
            label: display
            subtitle: toolTip

            property var thumbnail: attachmentPreview

            function startChat() {
                applicationWindow().pageStack.push(chatView, {
                                                       addresses: addresses,
                                                       conversationId: model.conversationId,
                                                       isMultitarget: isMultitarget,
                                                       initialMessage: page.initialMessage,
                                                       device: device})
                initialMessage = ""
            }

            onClicked: {
                startChat();
                view.currentIndex = index
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
                selected: (listItem.highlighted || listItem.checked || (listItem.pressed && listItem.supportsMouseEvents))
                opacity: 1
                visible: source != undefined
            }

            // Keep the currently-open chat highlighted even if this element is not focused
            highlighted: view.currentIndex == index
        }

        Component.onCompleted: {
            currentIndex = -1
            focus = true
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: deviceConnected && view.count == 0 && view.headerItem.childAt(0, 0).text.length != 0
            text: i18ndc("kdeconnect-sms", "Placeholder message text when no messages are found", "No matches")
        }
    }
}
