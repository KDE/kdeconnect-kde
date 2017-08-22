/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kdeconnect 1.0
import QtQuick.Controls.Styles 1.4

PlasmaComponents.ListItem
{
    id: root
    readonly property QtObject device: DeviceDbusInterfaceFactory.create(model.deviceId)

    RemoteKeyboard {
        id: remoteKeyboard
        device: root.device

        onRemoteStateChanged: {
            remoteKeyboardInput.available = remoteKeyboard.remoteState;
        }

        onKeyPressReceived: {
//            console.log("XXX received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + remoteKeyboardInput.text + " cursorPos=" + remoteKeyboardInput.cursorPosition);
            // interpret some special keys:
            if (specialKey == 12 || specialKey == 14)  // Return/Esc -> clear
                remoteKeyboardInput.text = "";
            else if (specialKey == 4  // Left
                     && remoteKeyboardInput.cursorPosition > 0)
                --remoteKeyboardInput.cursorPosition;
            else if (specialKey == 6  // Right
                     && remoteKeyboardInput.cursorPosition < remoteKeyboardInput.text.length)
                ++remoteKeyboardInput.cursorPosition;
            else if (specialKey == 1) {    // Backspace -> delete left
                var pos = remoteKeyboardInput.cursorPosition;
                if (pos > 0) {
                    remoteKeyboardInput.text = remoteKeyboardInput.text.substring(0, pos-1)
                                               + remoteKeyboardInput.text.substring(pos, remoteKeyboardInput.text.length);
                    remoteKeyboardInput.cursorPosition = pos - 1;
                }
            } else if (specialKey == 13) {    // Delete -> delete right
                var pos = remoteKeyboardInput.cursorPosition;
                if (pos < remoteKeyboardInput.text.length) {
                        remoteKeyboardInput.text = remoteKeyboardInput.text.substring(0, pos)
                          + remoteKeyboardInput.text.substring(pos+1, remoteKeyboardInput.text.length);
                    remoteKeyboardInput.cursorPosition = pos; // seems to be set to text.length automatically!
                }
            } else if (specialKey == 10)    // Home
                remoteKeyboardInput.cursorPosition = 0;
            else if (specialKey == 11)    // End
                remoteKeyboardInput.cursorPosition = remoteKeyboardInput.text.length;
            else {
                // echo visible keys
                var sanitized = "";
                for (var i = 0; i < key.length; i++) {
                    if (key.charCodeAt(i) > 31)
                        sanitized += key.charAt(i);
                }
                if (sanitized.length > 0 && !ctrl && !alt) {
                    // insert sanitized at current pos:
                    var pos = remoteKeyboardInput.cursorPosition;
                    remoteKeyboardInput.text = remoteKeyboardInput.text.substring(0, pos)
                                               + sanitized
                                               + remoteKeyboardInput.text.substring(pos, remoteKeyboardInput.text.length);
                    remoteKeyboardInput.cursorPosition = pos + 1; // seems to be set to text.length automatically!
                }
            }
//            console.log("XXX After received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + remoteKeyboardInput.text + " cursorPos=" + remoteKeyboardInput.cursorPosition);
        }
    }

    Column {
        width: parent.width
        
        RowLayout
        {
            Item {
                //spacer to make the label centre aligned in a row yet still elide and everything
                implicitWidth: (ring.visible? ring.width : 0) + (browse.visible? browse.width : 0) + parent.spacing
            }

            Battery {
                id: battery
                device: root.device
            }
            
            PlasmaComponents.Label {
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: (battery.available && battery.charge > -1) ? i18n("%1 (%2)", display, battery.displayString) : display
                Layout.fillWidth: true
                textFormat: Text.PlainText
            }

            //Find my phone
            PlasmaComponents.Button
            {
                FindMyPhone {
                    id: findmyphone
                    device: root.device
                }

                id: ring
                iconSource: "irc-voice"
                visible: findmyphone.available
                tooltip: i18n("Ring my phone")

                onClicked: {
                    findmyphone.ring()
                }
            }

            //SFTP
            PlasmaComponents.Button
            {
                Sftp {
                    id: sftp
                    device: root.device
                }

                id: browse
                iconSource: "document-open-folder"
                visible: sftp.available
                tooltip: i18n("Browse this device")

                onClicked: {
                    sftp.browse()
                }
            }

            height: browse.height
            width: parent.width
        }

        //RemoteKeyboard
        PlasmaComponents.ListItem {
            visible: remoteKeyboardInput.available
            width: parent.width

            Row {
                width: parent.width
                spacing: 5

                PlasmaComponents.Label {
                    id: remoteKeyboardLabel
                    text: i18n("Remote Keyboard")
                }

                PlasmaComponents.TextField {
                    id: remoteKeyboardInput

                    property bool available: remoteKeyboard.remoteState

                    textColor: "black"
                    height: parent.height
                    width: parent.width - 5 - remoteKeyboardLabel.width
                    verticalAlignment: TextInput.AlignVCenter
                    readOnly: !available
                    enabled: available
                    style: TextFieldStyle {
                        textColor: "black"
                        background: Rectangle {
                            radius: 2
                            border.color: "gray"
                            border.width: 1
                            color: remoteKeyboardInput.available ? "white" : "lightgray"
                        }
                    }

                    Keys.onPressed: {
                        if (remoteKeyboard.available)
                            remoteKeyboard.sendEvent(event);
                        event.accepted = true;
                    }
                }
            }
        }

        //Notifications
        PlasmaComponents.ListItem {
            visible: notificationsModel.count>0
            enabled: true
            PlasmaComponents.Label {
                text: i18n("Notifications:")
            }
            PlasmaComponents.ToolButton {
                enabled: true
                visible: notificationsModel.isAnyDimissable;
                anchors.right: parent.right
                iconSource: "window-close"
                onClicked: notificationsModel.dismissAll();
            }
        }
        Repeater {
            id: notificationsView
            model: NotificationsModel {
                id: notificationsModel
                deviceId: root.device.id()
            }
            delegate: PlasmaComponents.ListItem {
                id: listitem
                enabled: true
                onClicked: checked = !checked

                PlasmaCore.IconItem {
                    id: notificationIcon
                    source: appIcon
                    width: (valid && appIcon.length) ? dismissButton.width : 0
                    height: width
                    anchors.left: parent.left
                }
                PlasmaComponents.Label {
                    text: appName + ": " + (title.length>0 ? (appName==title?notitext:title+": "+notitext) : display)
                    anchors.right: replyButton.left
                    anchors.left: notificationIcon.right
                    elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                    maximumLineCount: listitem.checked ? 0 : 1
                    wrapMode: Text.WordWrap
                }
                PlasmaComponents.ToolButton {
                    id: replyButton
                    visible: repliable
                    enabled: repliable
                    anchors.right: dismissButton.left
                    iconSource: "mail-reply-sender"
                    onClicked: dbusInterface.reply();
                }
                PlasmaComponents.ToolButton {
                    id: dismissButton
                    visible: notificationsModel.isAnyDimissable;
                    enabled: dismissable
                    anchors.right: parent.right
                    iconSource: "window-close"
                    onClicked: dbusInterface.dismiss();
                }
            }
        }

        //NOTE: More information could be displayed here

    }
}
