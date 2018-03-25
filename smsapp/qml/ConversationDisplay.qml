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

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage
{
    id: page
    readonly property string phoneNumber: person.contactCustomProperty("phoneNumber")
    title: i18n("%1: %2", person.name, phoneNumber)
    property QtObject person
    property QtObject device

    readonly property QtObject telephony: TelephonyDbusInterfaceFactory.create(device.id())

    Kirigami.CardsListView {
        model: ListModel {
            ListElement { display: "aaa"; fromMe: true }
            ListElement { display: "aaa" }
            ListElement { display: "aaa"; fromMe: true }
            ListElement { display: "aaa" }
            ListElement { display: "aaa" }
            ListElement { display: "aaa" }
        }
        delegate: Kirigami.AbstractCard {
            readonly property real margin: 100
            x: fromMe ? Kirigami.Units.gridUnit : margin
            width: parent.width - margin - Kirigami.Units.gridUnit
            contentItem: Label { text: model.display }
        }
    }
    footer: RowLayout {
        TextField {
            id: message
            Layout.fillWidth: true
            placeholderText: i18n("Say hi...")
        }
        Button {
            text: "Send"
            onClicked: {
                console.log("sending sms", page.phoneNumber)
                page.telephony.sendSms(page.phoneNumber, message.text)
            }
        }
    }
}
