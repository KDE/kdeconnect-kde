/**
 * SPDX-FileCopyrightText: 2017 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.kdeconnect 1.0
import QtQuick.Controls 2.4

TextField {

    id: root

    property alias device: checker.device
    readonly property alias available: checker.available

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "remotekeyboard"
    }

    property var remoteKeyboard: null

    readonly property bool remoteState: available ? remoteKeyboard.remoteState : false

    Connections {
        target: remoteKeyboard
        function onKeyPressReceived() {
            //console.log("XXX received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + text + " cursorPos=" + cursorPosition);
            // interpret some special keys:
            if (specialKey == 12 || specialKey == 14)  // Return/Esc -> clear
                text = "";
            else if (specialKey == 4  // Left
                     && cursorPosition > 0)
                --cursorPosition;
            else if (specialKey == 6  // Right
                     && cursorPosition < text.length)
                ++cursorPosition;
            else if (specialKey == 1) {    // Backspace -> delete left
                var pos = cursorPosition;
                if (pos > 0) {
                    text = text.substring(0, pos-1)
                                               + text.substring(pos, text.length);
                    cursorPosition = pos - 1;
                }
            } else if (specialKey == 13) {    // Delete -> delete right
                var pos = cursorPosition;
                if (pos < text.length) {
                        text = text.substring(0, pos)
                          + text.substring(pos+1, text.length);
                    cursorPosition = pos; // seems to be set to text.length automatically!
                }
            } else if (specialKey == 10)    // Home
                cursorPosition = 0;
            else if (specialKey == 11)    // End
                cursorPosition = text.length;
            else {
                // echo visible keys
                var sanitized = "";
                for (var i = 0; i < key.length; i++) {
                    if (key.charCodeAt(i) > 31)
                        sanitized += key.charAt(i);
                }
                if (sanitized.length > 0 && !ctrl && !alt) {
                    // insert sanitized at current pos:
                    var pos = cursorPosition;
                    text = text.substring(0, pos)
                                               + sanitized
                                               + text.substring(pos, text.length);
                    cursorPosition = pos + 1; // seems to be set to text.length automatically!
                }
            }
//            console.log("XXX After received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + text + " cursorPos=" + cursorPosition);
        }
    }


    function sendEvent(event) {
        if (remoteKeyboard) {
            var transEvent = JSON.parse(JSON.stringify(event)); // transform to anonymous object
            remoteKeyboard.sendQKeyEvent(transEvent);
            event.accepted = true
        }
    }

    Keys.onPressed: {
        if (available)
            sendEvent(event);
        event.accepted = true;
    }

    onAvailableChanged: {
        if (available) {
            remoteKeyboard = RemoteKeyboardDbusInterfaceFactory.create(device.id());
            //remoteKeyboard.keyPressReceived.connect(keyPressReceived);
            remoteKeyboard.remoteStateChanged.connect(remoteStateChanged);
        } else {
            remoteKeyboard = null
        }
    }
}
