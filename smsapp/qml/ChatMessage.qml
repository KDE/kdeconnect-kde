/**
 * SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents

Item {
    id: root

    height: Math.max(avatar.height, messageBubble.height)

    property string messageBody
    property bool sentByMe
    property string selectedText
    property date dateTime
    property string name
    property bool multiTarget
    property var attachmentList: []

    signal messageCopyRequested(string message)

    KirigamiComponents.Avatar {
        id: avatar
        width: visible ? Kirigami.Units.gridUnit * 2 : 0
        height: width
        visible: !root.sentByMe && multiTarget
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
            id: messageBox
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

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: mouse => {
                    if (mouse.button === Qt.RightButton) {
                        var selectStart = messageLabel.selectionStart;
                        var selectEnd = messageLabel.selectionEnd;
                        selectedText = messageLabel.selectedText;
                        contextMenu.x = mouse.x;
                        contextMenu.y = mouse.y;
                        messageLabel.persistentSelection = true;
                        contextMenu.open();
                    }
                }
                onPressAndHold: mouse => {
                    var selectStart = messageLabel.selectionStart;
                    var selectEnd = messageLabel.selectionEnd;
                    selectedText = messageLabel.selectedText;
                    contextMenu.x = mouse.x;
                    contextMenu.y = mouse.y;
                    messageLabel.persistentSelection = true;
                    contextMenu.open();
                }
            }

            Column {
                id: messageColumn
                width: parent.width
                height: childrenRect.height

                property int contentWidth: Math.max(Math.max(messageLabel.implicitWidth, attachmentGrid.implicitWidth), dateLabel.implicitWidth)
                Label {
                    id: authorLabel
                    width: parent.width
                    text: root.name
                    leftPadding: Kirigami.Units.largeSpacing
                    topPadding: Kirigami.Units.smallSpacing
                    visible: multiTarget
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: messageLabel.horizontalAlignment
                }

                Grid {
                    id: attachmentGrid
                    columns: 2
                    padding: attachmentList.length > 0 ? Kirigami.Units.largeSpacing : 0
                    layoutDirection: root.sentByMe ? Qt.RightToLeft : Qt.LeftToRight

                    Repeater {
                        model: attachmentList

                        delegate: MessageAttachments {
                            mimeType: modelData.mimeType
                            partID: modelData.partID
                            uniqueIdentifier: modelData.uniqueIdentifier
                        }
                    }
                }

                TextEdit {
                    id: messageLabel
                    visible: messageBody != ""
                    selectByMouse: true
                    readOnly: true
                    leftPadding: Kirigami.Units.largeSpacing
                    rightPadding: Kirigami.Units.largeSpacing
                    topPadding: authorLabel.visible ? 0 : Kirigami.Units.largeSpacing
                    width: parent.width
                    horizontalAlignment: root.sentByMe ? Text.AlignRight : Text.AlignLeft
                    wrapMode: Text.Wrap
                    color: Kirigami.Theme.textColor
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

            Menu {
                id: contextMenu
                exit: Transition {PropertyAction { target: messageLabel; property: "persistentSelection"; value: false }}
                MenuItem {
                    text: i18nd("kdeconnect-sms", "Copy Message")
                    enabled: messageLabel.visible
                    onTriggered: root.messageCopyRequested(root.messageBody)
                }
                MenuItem {
                    text: i18nd("kdeconnect-sms", "Copy Selection")
                    visible: selectedText != ""
                    onTriggered: {
                            root.messageCopyRequested(selectedText)
                    }
                }

            }
        }
    }
}
