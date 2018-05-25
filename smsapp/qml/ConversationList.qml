/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

import QtQuick 2.5
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.people 1.0
import org.kde.plasma.core 2.0 as Core
import org.kde.kirigami 2.2 as Kirigami
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.sms 1.0

Kirigami.ScrollablePage
{
    footer: ComboBox {
        id: devicesCombo
        enabled: count > 0
        displayText: enabled ? undefined : i18n("No devices available")
        model: DevicesSortProxyModel {
            id: devicesModel
            //TODO: make it possible to sort only if they can do sms
            sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
            onRowsInserted: if (devicesCombo.currentIndex < 0) {
                devicesCombo.currentIndex = 0
            }
        }
        textRole: "display"
    }

    readonly property QtObject device: devicesCombo.currentIndex >= 0 ? devicesModel.data(devicesModel.index(devicesCombo.currentIndex, 0), DevicesModel.DeviceRole) : null

    Component {
        id: chatView
        ConversationDisplay {}
    }

    ListView {
        id: view
        currentIndex: 0

        model: QSortFilterProxyModel {
            sortOrder: Qt.DescendingOrder
            sortRole: ConversationListModel.DateRole
            sourceModel: ConversationListModel {
                deviceId: device ? device.id() : ""
            }
        }

        header: TextField {
            id: filter
            placeholderText: i18n("Filter...")
            width: parent.width
            onTextChanged: {
                view.model.filterRegExp = new RegExp(filter.text)
                view.currentIndex = 0
            }
            Keys.onUpPressed: view.currentIndex = Math.max(view.currentIndex-1, 0)
            Keys.onDownPressed: view.currentIndex = Math.min(view.currentIndex+1, view.count-1)
            onAccepted: {
                view.currentItem.startChat()
            }
            Shortcut {
                sequence: "Ctrl+F"
                onActivated: filter.forceActiveFocus()
            }
        }

        delegate: Kirigami.BasicListItem
        {
            hoverEnabled: true

            label: i18n("<b>%1</b> - %2", display, toolTip)
            icon: decoration
            function startChat() {
                applicationWindow().pageStack.push(chatView, {
                                                       personUri: model.personUri,
                                                       phoneNumber: address,
                                                       conversationId: model.conversationId,
                                                       device: device})
            }
            onClicked: { startChat(); }
        }

    }
}
