/**
 * Copyright 2017 Holger Kaelberer <holger.k@elberer.de>
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

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
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

    property variant remoteKeyboard: null

    readonly property bool remoteState: available ? remoteKeyboard.remoteState : false

    Connections {
        target: remoteKeyboard
        onKeyPressReceived: {
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
            if (transEvent.modifiers & Qt.ControlModifier) {
                // special handling for ctrl+c/v/x/a, for which only 'key' gets
                // set, but no visible 'text', which is expected by the remoteKeyboard
                // wire-format:
                if (transEvent.key === Qt.Key_C)
                    transEvent.text = 'c';
                if (transEvent.key === Qt.Key_V)
                    transEvent.text = 'v';
                if (transEvent.key === Qt.Key_A)
                    transEvent.text = 'a';
                if (transEvent.key === Qt.Key_X)
                    transEvent.text = 'x';
            }
            remoteKeyboard.sendQKeyEvent(transEvent);
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
