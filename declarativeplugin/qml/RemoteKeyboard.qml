/**
 * SPDX-FileCopyrightText: 2017 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.kdeconnect as KDEConnect

QQC2.TextField {
    id: root

    property KDEConnect.DeviceDbusInterface device

    readonly property alias available: checker.available

    readonly property KDEConnect.PluginChecker pluginChecker: KDEConnect.PluginChecker {
        id: checker
        pluginName: "remotekeyboard"
        device: root.device
    }

    property KDEConnect.RemoteKeyboardDbusInterface remoteKeyboard

    readonly property bool remoteState: available && remoteKeyboard ? remoteKeyboard.remoteState : false

    Connections {
        target: root.remoteKeyboard

        function onKeyPressReceived(key: string, specialKey: int, shift: bool, ctrl: bool, alt: bool): void {
            // console.log("XXX received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + text + " cursorPos=" + cursorPosition);
            // interpret some special keys:
            if (specialKey === 12 || specialKey === 14) { // Return/Esc -> clear
                text = "";
            } else if (specialKey === 4 // Left
                     && cursorPosition > 0) {
                --cursorPosition;
            } else if (specialKey === 6 // Right
                     && cursorPosition < text.length) {
                ++cursorPosition;
            } else if (specialKey === 1) {  // Backspace -> delete left
                const pos = cursorPosition;
                if (pos > 0) {
                    text = text.substring(0, pos - 1)
                        + text.substring(pos, text.length);
                    cursorPosition = pos - 1;
                }
            } else if (specialKey === 13) { // Delete -> delete right
                const pos = cursorPosition;
                if (pos < text.length) {
                    text = text.substring(0, pos)
                        + text.substring(pos + 1, text.length);
                    cursorPosition = pos; // seems to be set to text.length automatically!
                }
            } else if (specialKey === 10) { // Home
                cursorPosition = 0;
            } else if (specialKey == 11) { // End
                cursorPosition = text.length;
            } else {
                // echo visible keys
                let sanitized = "";
                for (let i = 0; i < key.length; i++) {
                    if (key.charCodeAt(i) > 31) {
                        sanitized += key.charAt(i);
                    }
                }
                if (sanitized.length > 0 && !ctrl && !alt) {
                    // insert sanitized at current pos:
                    const pos = cursorPosition;
                    text = text.substring(0, pos)
                        + sanitized
                        + text.substring(pos, text.length);
                    cursorPosition = pos + 1; // seems to be set to text.length automatically!
                }
            }
            // console.log("XXX After received keypress key=" + key + " special=" + specialKey + " shift=" + shift + " ctrl=" + ctrl + " text=" + text + " cursorPos=" + cursorPosition);
        }
    }

    KDEConnect.KeyListener {
        target: checker.available && root.remoteKeyboard ? root : null
        onKeyReleased: event => {
            remoteKeyboard.sendQKeyEvent(event);
            event.accepted = true
        }
    }

    onAvailableChanged: {
        if (available && device !== null) {
            remoteKeyboard = KDEConnect.RemoteKeyboardDbusInterfaceFactory.create(device.id());
            remoteKeyboard.remoteStateChanged.connect(remoteStateChanged);
        } else {
            remoteKeyboard = null;
        }
    }
}
