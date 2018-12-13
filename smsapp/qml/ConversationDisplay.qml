/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
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
import QtQuick.Controls 2.4
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
    property QtObject device
    property int conversationId

    property string phoneNumber
    title: person.person && person.person.name ? person.person.name : phoneNumber

    function sendMessage() {
        console.log("sending sms", page.phoneNumber)
        model.sourceModel.sendReplyToConversation(message.text)
        message.text = ""
    }

    ListView {
        model: QSortFilterProxyModel {
            id: model
            sortOrder: Qt.AscendingOrder
            sortRole: ConversationModel.DateRole
            sourceModel: ConversationModel {
                deviceId: device.id()
                threadId: page.conversationId
            }
        }

        spacing: Kirigami.Units.largeSpacing

        delegate: ChatMessage {
            messageBody: model.display
            sentByMe: model.fromMe
            dateTime: new Date(model.date)
        }

        // Set the view to start at the bottom of the page and track new elements if it was not manually scrolled up
        currentIndex: atYEnd ?
                        count - 1 :
                        currentIndex
    }

    footer: RowLayout {
        enabled: page.device
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
