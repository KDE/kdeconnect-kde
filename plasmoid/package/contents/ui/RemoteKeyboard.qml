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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kdeconnect 1.0

QtObject {

    id: root

    property alias device: checker.device
    readonly property alias available: checker.available

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "remotekeyboard"
    }

    property variant remoteKeyboard: null

    readonly property bool remoteState: available ? remoteKeyboard.remoteState : false

    signal keyPressReceived(string key, int specialKey, bool shift, bool ctrl, bool alt)

    function sendEvent(event) {
        if (remoteKeyboard) {
            var transEvent = JSON.parse(JSON.stringify(event)); // transform to anonymous object
            if (transEvent.modifiers & Qt.ControlModifier) {
                // special handling for ctrl+c/v/x/a, for which only 'key' gets
                // set, but no visbile 'text', which is expected by the remoteKeyboard
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

    onAvailableChanged: {
        if (available) {
            remoteKeyboard = RemoteKeyboardDbusInterfaceFactory.create(device.id());
            remoteKeyboard.keyPressReceived.connect(keyPressReceived);
            remoteKeyboard.remoteStateChanged.connect(remoteStateChanged);
        } else {
            remoteKeyboard = null
        }
    }
}
