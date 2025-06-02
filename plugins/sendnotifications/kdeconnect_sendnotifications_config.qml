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

Kirigami.ScrollablePage {
    id: root

    property string device

    Kirigami.FormLayout {
        KdeConnectPluginConfig {
            id: config
            deviceId: root.device
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
            focus: true
            onClicked: config.set("generalPersistent", checked)
            KeyNavigation.down: includeBody
        }

        CheckBox {
            id: includeBody
            text: i18n("Include body")
            onClicked: config.set("generalIncludeBody", checked)
            KeyNavigation.down: includeIcon
        }

        CheckBox {
            id: includeIcon
            text: i18n("Include icon")
            onClicked: config.set("generalSynchronizeIcons", checked)
            KeyNavigation.down: urgency
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
