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

import QtQuick 2.5
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.people 1.0
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.sms 1.0

Kirigami.ScrollablePage
{
    id: page
    ToolTip {
        id: noDevicesWarning
        visible: !devicesCombo.enabled
        timeout: -1
        text: "⚠️ " + i18n("No devices available") + " ⚠️"

        MouseArea {
            // Detect mouseover and show another tooltip with more information
            anchors.fill: parent
            hoverEnabled: true

            ToolTip.visible: containsMouse
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            // TODO: Wrap text if line is too long for the screen
            ToolTip.text: i18n("No new messages can be sent or received, but you can browse cached content")
        }
    }

    property string initialMessage

    header: Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: page.initialMessage.length > 0
        text: i18n("Choose recipient")

        actions: [
          Kirigami.Action {
              iconName: "dialog-cancel"
              text: "Cancel"
              onTriggered: initialMessage = ""
            }
        ]
    }

    footer: ComboBox {
        id: devicesCombo
        enabled: count > 0
        displayText: !enabled ? i18n("No devices available") : undefined
        model: DevicesSortProxyModel {
            id: devicesModel
            //TODO: make it possible to filter if they can do sms
            sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
            onRowsInserted: if (devicesCombo.currentIndex < 0) {
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
            sortRole: ConversationListModel.DateRole
            filterCaseSensitivity: Qt.CaseInsensitive
            sourceModel: ConversationListModel {
                id: conversationListModel
                deviceId: device ? device.id() : ""
            }
        }

        header: TextField {
            /**
             * Used as the filter of the list of messages
             */
            id: filter
            placeholderText: i18n("Filter...")
            width: parent.width
            z: 10
            onTextChanged: {
                view.model.setFilterFixedString(filter.text);
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
        headerPositioning: ListView.OverlayHeader

        Keys.forwardTo: [headerItem]

        delegate: Kirigami.BasicListItem
        {
            hoverEnabled: true

            label: i18n("<b>%1</b> <br> %2", display, toolTip)
            icon: decoration
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
            // Keep the currently-open chat highlighted even if this element is not focused
            highlighted: chatView.conversationId == model.conversationId
        }

        Component.onCompleted: {
            currentIndex = -1
            focus = true
        }
    }
}
