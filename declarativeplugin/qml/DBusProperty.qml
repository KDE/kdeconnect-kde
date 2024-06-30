/**
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQml

import org.kde.kdeconnect as KDEConnect

QtObject {
    id: prop

    property QtObject object
    property string read
    property string change: read + "Changed"

    Component.onCompleted: {
        get();
    }

    onChangeChanged: {
        if (object) {
            const signal = object[change];
            if (signal) {
                signal.connect(valueReceived);
            } else {
                console.warn(`couldn't find signal ${change} for ${object}`);
            }
        }
    }

    function valueReceived(value: var): void {
        if (!value) {
            get();
        } else {
            _value = value;
        }
    }

    property var defaultValue
    property var _value: defaultValue
    readonly property var value: _value

    readonly property KDEConnect.DBusAsyncResponse __response: KDEConnect.DBusAsyncResponse {
        id: response

        autoDelete: false

        onSuccess: result => {
            prop._value = result;
        }

        onError: message => {
            console.warn("failed call", prop.object, prop.read, prop.change, message);
        }
    }

    function get(): void {
        if (object) {
            const method = object[read];
            if (method) {
                response.setPendingCall(method());
            }
        }
    }
}
