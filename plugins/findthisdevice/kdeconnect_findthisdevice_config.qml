/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import Qt.labs.platform 1.1
import org.kde.kdeconnect 1.0

Kirigami.Page {

    property string device

    actions: [
        Kirigami.Action {
            icon.name: "dialog-ok"
            text: i18n("Apply")
            onTriggered: config.set("ringtone", path.text)
        }
    ]

    Kirigami.FormLayout {

        anchors.fill: parent

        FileDialog {
            id: fileDialog
            currentFile: path.text

            onAccepted: {
                path.text = currentFile.toString().replace("file://", "")
            }
        }

        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_findthisdevice"

            onConfigChanged: {
                path.text = getString("ringtone", StandardPaths.writableLocation(StandardPaths.DownloadsLocation).toString().replace("file://", ""))
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Sound to play:")

            QQC2.TextField {
                id: path
            }

            QQC2.Button {
                icon.name: "document-open"
                onClicked: fileDialog.open()
            }
        }
    }
}
