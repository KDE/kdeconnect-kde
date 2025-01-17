/*
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect

Kirigami.Page
{
    id: root
    title: i18nd("kdeconnect-app", "Volume control")
    property QtObject pluginInterface
    property QtObject device

    ListView {
        id: sinkList
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing
        focus: true

        model: RemoteSinksModel {
            deviceId: pluginInterface.deviceId
        }
        delegate: ColumnLayout {

            width: parent.width
            onActiveFocusChanged: if (activeFocus) muteButton.forceActiveFocus(Qt.TabFocusReason)

            Label {
                text: description
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            RowLayout {

                Button {
                    icon.name: muted ? "player-volume-muted" : "player-volume"
                    onClicked: muted = !muted
                }

                Slider {
                    Layout.fillWidth: true
                    from: 0
                    value: volume
                    to: maxVolume
                    onMoved: volume = value
                }
            }
        }
    }
}

