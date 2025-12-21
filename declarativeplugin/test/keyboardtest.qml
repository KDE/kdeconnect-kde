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

Window {
    width: 400
    height: 400
    visible: true

    QQC2.TextField {
        id: root

        KDEConnect.KeyListener {
            target: root
            onKeyReleased: event => {
                remoteKeyboard.sendQKeyEvent(event);
                console.log("event", JSON.stringify(event))
                event.accepted = true
            }
        }
    }
}
