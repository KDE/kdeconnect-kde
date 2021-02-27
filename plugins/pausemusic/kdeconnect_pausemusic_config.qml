/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.5 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.FormLayout {

    property string device

    KdeConnectPluginConfig {
        id: config
        deviceId: device
        pluginName: "kdeconnect_pausemusic"
    }

    Component.onCompleted: {
        talking.checked = config.get("conditionTalking", false)
        mute.checked = config.get("actionMute", false)
        pause.checked = config.get("actionPause", true)
    }

    RadioButton {
        text: i18n("Pause as soon as phone rings")
        checked: !talking.checked
    }

    RadioButton {
        id: talking
        onCheckedChanged: config.set("conditionTalking", checked)
        text: i18n("Pause only while talking")
    }

    CheckBox {
        id: pause
        text: i18n("Pause media players")
        onClicked: config.set("actionPause", checked)
    }

    CheckBox {
        id: mute
        text: i18n("Mute system sound")
        onClicked: config.set("actionMute", checked)
    }

}
