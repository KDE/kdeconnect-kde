/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import QtCore
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage {
    id: root

    property string device

    actions: Kirigami.Action {
        icon.name: "dialog-ok"
        text: i18n("Apply")
        onTriggered: config.set("incoming_path", path.text)
    }

    Kirigami.FormLayout {
        FolderDialog {
            id: folderDialog
            currentFolder: path.text

            onAccepted: {
                path.text = selectedFolder.toString().replace("file://", "")
            }
        }

        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_share"

            onConfigChanged: {
                path.text = getString("incoming_path", StandardPaths.writableLocation(StandardPaths.DownloadsLocation).toString().replace("file://", ""))
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Save files in:")

            QQC2.TextField {
                id: path
            }

            QQC2.Button {
                icon.name: "document-open"
                onClicked: folderDialog.open()
            }
        }

        QQC2.Label {
            text: i18n("%1 in the path will be replaced with the specific device name", "%1")
        }
    }
}
