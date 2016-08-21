/**
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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
    id: prop
    property QtObject object: null
    property string read
    property string change: read+"Changed"

    Component.onCompleted: get();

    onChangeChanged: {
        if (object) {
            var theSignal = object[change];
            if (theSignal) {
                theSignal.connect(valueReceived);
            } else {
                console.warn("couldn't find signal", change, "for", object)
            }
        }
    }

    function valueReceived(val) {
        if (!val) {
            get();
        } else {
            _value = val;
        }
    }

    property var defaultValue
    property var _value: defaultValue
    readonly property var value: _value

    readonly property var v: DBusAsyncResponse {
        id: response
        autoDelete: false
        onSuccess: {
            prop._value = result;
        }
        onError: {
            console.warn("failed call", object, read, write, change)
        }
    }

    function get() {
        response.setPendingCall(object[read]());
    }
}
