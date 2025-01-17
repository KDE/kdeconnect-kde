/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import QtQuick.Dialogs as Dialogs
import QtMultimedia
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.private.findthisdevice as FindThisDevice

Kirigami.ScrollablePage {
    id: root

    property string device

    actions: Kirigami.Action {
        icon.name: "dialog-ok"
        text: i18n("Apply")
        onTriggered: config.set("ringtone", path.text)
    }

    Kirigami.FormLayout {
        Dialogs.FileDialog {
            id: fileDialog
            selectedFile: "file://" + path.text
            onAccepted: {
                path.text = selectedFile.toString().replace("file://", "")
            }
        }

        KdeConnectPluginConfig {
            id: config
            deviceId: device
            pluginName: "kdeconnect_findthisdevice"

            onConfigChanged: {
                path.text = getString("ringtone", FindThisDevice.FindThisDeviceHelper.defaultSound())
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Sound to play:")

            QQC2.TextField {
                id: path
                focus: true
                KeyNavigation.right: filePickerButton
            }

            QQC2.Button {
                id: filePickerButton
                text: i18nc("@action:button", "Choose file")
                display: QQC2.Button.IconOnly
                icon.name: "document-open-symbolic"
                onClicked: fileDialog.open()
                KeyNavigation.right: playButton
            }

            QQC2.Button {
                id: playButton
                text: i18nc("@action:button", "Play")
                display: QQC2.Button.IconOnly
                icon.name: playMusic.playing ? "media-playback-stop-symbolic" : "media-playback-start-symbolic"
                enabled: FindThisDevice.FindThisDeviceHelper.pathExists(path.text)
                onClicked: {
                    playMusic.source = path.text;
                    playMusic.playing ? playMusic.stop() : playMusic.play();
                }

                MediaPlayer {
                    id: playMusic
                    audioOutput: AudioOutput {}
                }
            }
        }
    }
}
