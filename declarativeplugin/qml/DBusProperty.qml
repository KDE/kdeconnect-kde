/**
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
        onSuccess: result => {
            prop._value = result;
        }
        onError: message => {
            console.warn("failed call", prop.object, prop.read, prop.change, message)
        }
    }

    function get() {
        response.setPendingCall(object[read]());
    }
}
