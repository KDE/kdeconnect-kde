/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.Page {

    property string device

    Kirigami.FormLayout {

        anchors.fill: parent

        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_pausemusic"
        }

        Component.onCompleted: {
            talking.checked = config.getBool("conditionTalking", false)
            mute.checked = config.getBool("actionMute", false)
            pause.checked = config.getBool("actionPause", true)
            resume.checked = config.getBool("actionResume", true)
        }

        QQC2.RadioButton {
            text: i18n("Pause as soon as phone rings")
            checked: !talking.checked
        }

        QQC2.RadioButton {
            id: talking
            onCheckedChanged: config.set("conditionTalking", checked)
            text: i18n("Pause only while talking")
        }

        QQC2.CheckBox {
            id: pause
            text: i18n("Pause media players")
            onClicked: config.set("actionPause", checked)
        }

        QQC2.CheckBox {
            id: mute
            text: i18n("Mute system sound")
            onClicked: config.set("actionMute", checked)
        }

        QQC2.CheckBox {
            id: resume
            text: i18n("Resume automatically when call ends")
            onClicked: config.set("actionResume", checked)
        }
    }
}
