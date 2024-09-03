/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 * SPDX-FileCopyrightText: 2016 David Kahles <david.kahles96@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQml 2.2
import org.kde.kdeconnect 1.0

QtObject {

    id: root

    property alias device: conn.target
    property string pluginName: ""
    property bool available: false
    property string iconName: ""

    readonly property Connections connection: Connections {
        id: conn
        function onPluginsChanged() {
            root.pluginsChanged();
        }
    }

    Component.onCompleted: pluginsChanged()

    readonly property var v: DBusAsyncResponse {
        id: availableResponse
        autoDelete: false
        onSuccess: (result) => { root.available = result; }
        onError: () => { root.available = false }
    }

    function pluginsChanged() {
        availableResponse.setPendingCall(device.hasPlugin("kdeconnect_" + pluginName))
        iconResponse.setPendingCall(device.pluginIconName("kdeconnect_" + pluginName))
    }

    readonly property var vv: DBusAsyncResponse {
        id: iconResponse
        autoDelete: false
        onSuccess: (result) => { root.iconName = result; }
        onError: () => { root.iconName = "" }
    }
}
