/**
 * Copyright (C) 2020 Nicolas Fella <nicolas.fella@gmx.de>
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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import org.kde.kirigami 2.13 as Kirigami

Item {
    id: root

    height: Math.max(avatar.height, messageBubble.height)

    property string messageBody
    property bool sentByMe
    property date dateTime
    property string name

    signal messageCopyRequested(string message)

    Kirigami.Avatar {
        id: avatar
        width: Kirigami.Units.gridUnit * 2
        height: width
        visible: !root.sentByMe
        name: root.name

        anchors.left: parent.left
        anchors.leftMargin: Kirigami.Units.largeSpacing
    }

    RowLayout {
        id: messageBubble

        anchors.right: parent.right
        anchors.rightMargin: Kirigami.Units.largeSpacing
        anchors.left: avatar.right
        anchors.leftMargin: Kirigami.Units.largeSpacing

        height: messageColumn.height

        Rectangle {
            Layout.maximumWidth: applicationWindow().wideScreen ? Math.min(messageColumn.contentWidth, root.width * 0.6) : messageColumn.contentWidth
            Layout.fillWidth: true
            Layout.alignment: root.sentByMe ? Qt.AlignRight : Qt.AlignLeft
            Layout.fillHeight: true
            Layout.minimumWidth: dateLabel.implicitWidth

            color: {
                Kirigami.Theme.colorSet = Kirigami.Theme.View
                var accentColor = Kirigami.Theme.highlightColor
                return Qt.tint(Kirigami.Theme.backgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, root.sentByMe ? 0.1 : 0.4))
            }
            radius: 6

            Column {
                id: messageColumn
                width: parent.width
                height: childrenRect.height

                property int contentWidth: Math.max(messageLabel.implicitWidth, dateLabel.implicitWidth)

                Label {
                    id: messageLabel
                    leftPadding: Kirigami.Units.largeSpacing
                    rightPadding: Kirigami.Units.largeSpacing
                    topPadding: Kirigami.Units.largeSpacing
                    width: parent.width
                    horizontalAlignment: root.sentByMe ? Text.AlignRight : Text.AlignLeft
                    wrapMode: Text.Wrap
                    text: root.messageBody
                }

                Label {
                    id: dateLabel
                    width: parent.width
                    text: Qt.formatDateTime(root.dateTime, "dd. MMM, hh:mm")
                    leftPadding: Kirigami.Units.largeSpacing
                    rightPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.smallSpacing
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: messageLabel.horizontalAlignment
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: mouse => {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup()
                    }
                }
                onPressAndHold: contextMenu.popup()
            }

            Menu {
                id: contextMenu

                MenuItem {
                    text: i18nd("kdeconnect-sms", "Copy Message")
                    enabled: messageLabel.visible
                    onTriggered: root.messageCopyRequested(root.messageBody)
                }
            }
        }
    }
}
