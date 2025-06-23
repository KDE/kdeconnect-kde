/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage {
    id: root

    property string device
    readonly property string max_clipboard_file_size: "maxClipboardFileSizeMB"

    Component.onCompleted: {
        autoShare.checked = config.getBool("autoShare", config.getBool("sendUnknown", true))
        password.checked = config.getBool("sendPassword", true)
        maxClipboardFileSizeMB.value = config.getInt(max_clipboard_file_size, 50)
    }

    Kirigami.FormLayout {
        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_clipboard"
        }

        QQC2.CheckBox {
            id: autoShare
            text: i18n("Automatically share the clipboard from this device")
            focus: true
            onClicked: config.set("autoShare", checked)
            KeyNavigation.down: password
        }

        QQC2.CheckBox {
            id: password
            text: i18n("Including passwords (as marked by password managers)")
            onClicked: config.set("sendPassword", checked)
        }

        RowLayout {          
            QQC2.SpinBox {
                id: maxClipboardFileSizeMB
                from: 0
                to: 1000000
                onValueModified: config.set(max_clipboard_file_size, value)
            }

            QQC2.Label {
                text: i18n("Max Clipboard File Size (MB)")
            }
        }
    }
}
