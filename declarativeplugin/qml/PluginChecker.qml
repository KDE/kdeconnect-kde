/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 * SPDX-FileCopyrightText: 2016 David Kahles <david.kahles96@gmail.com>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQml

import org.kde.kdeconnect as KDEConnect

QtObject {
    id: root

    property KDEConnect.DeviceDbusInterface device
    property string pluginName
    property bool available
    property string iconName

    readonly property Connections connection: Connections {
        id: conn

        target: root.device

        function onPluginsChanged(): void {
            root.pluginsChanged();
        }
    }

    Component.onCompleted: {
        pluginsChanged();
    }

    readonly property KDEConnect.DBusAsyncResponse __availableResponse: KDEConnect.DBusAsyncResponse {
        id: availableResponse

        autoDelete: false

        onSuccess: result => {
            root.available = result;
        }

        onError: message => {
            root.available = false;
        }
    }

    function pluginsChanged(): void {
        if (device) {
            availableResponse.setPendingCall(device.hasPlugin("kdeconnect_" + pluginName));
            iconResponse.setPendingCall(device.pluginIconName("kdeconnect_" + pluginName));
        }
    }

    readonly property KDEConnect.DBusAsyncResponse __iconResponse: KDEConnect.DBusAsyncResponse {
        id: iconResponse

        autoDelete: false

        onSuccess: result => {
            root.iconName = result;
        }

        onError: message => {
            root.iconName = "";
        }
    }
}
