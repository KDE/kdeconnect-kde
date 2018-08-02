/**
 * Copyright 2014 Samoilenko Yuri <kinnalru@gmail.com>
 * Copyright 2016 David Kahles <david.kahles96@gmail.com>
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

import QtQml 2.2
import org.kde.kdeconnect 1.0

QtObject {

    id: root

    property alias device: conn.target
    property string pluginName: ""
    property bool available: false

    readonly property Connections connection: Connections {
        id: conn
        onPluginsChanged: pluginsChanged()
    }

    Component.onCompleted: pluginsChanged()

    readonly property var v: DBusAsyncResponse {
        id: response
        autoDelete: false
        onSuccess: { root.available = result; }
        onError: { root.available = false }
    }

    function pluginsChanged() {
        response.setPendingCall(device.hasPlugin("kdeconnect_" + pluginName))
    }
}
