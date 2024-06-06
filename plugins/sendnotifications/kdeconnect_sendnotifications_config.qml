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

Kirigami.Page {

    property string device

    Kirigami.FormLayout {

        anchors.fill: parent

        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_sendnotifications"
        }

        Component.onCompleted: {
            persistent.checked = config.getBool("generalPersistent", false)
            includeBody.checked = config.getBool("generalIncludeBody", true)
            includeIcon.checked = config.getBool("generalSynchronizeIcons", true)
            urgency.value = config.getInt("generalUrgency", 0)
        }

        CheckBox {
            id: persistent
            text: i18n("Persistent notifications only")
            onClicked: config.set("generalPersistent", checked)
        }

        CheckBox {
            id: includeBody
            text: i18n("Include body")
            onClicked: config.set("generalIncludeBody", checked)
        }

        CheckBox {
            id: includeIcon
            text: i18n("Include icon")
            onClicked: config.set("generalSynchronizeIcons", checked)
        }

        SpinBox {
            id: urgency
            Kirigami.FormData.label: i18n("Minimum urgency level:")
            from: 0
            to: 2
            onValueModified: config.set("generalUrgency", value)
        }

    }
}
