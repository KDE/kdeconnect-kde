/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect

ScrollView {
    id: root

    focus: true

    signal clicked(string device)

    property string currentDeviceId

    property alias model: devices.model

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
        radius: Kirigami.Units.cornerRadius

        border.color: Kirigami.ColorUtils.linearInterpolation(
            Kirigami.Theme.backgroundColor,
            Kirigami.Theme.textColor,
            Kirigami.Theme.frameContrast
        )
    }

    ListView {
        id: devices

        focus: true

        section {
            property: "status"
            delegate: Kirigami.ListSectionHeader {

                width: ListView.view.width

                text: switch (parseInt(section))
                {
                    case DevicesModel.Paired:
                        return i18nd("kdeconnect-kcm", "Remembered")
                    case DevicesModel.Reachable:
                        return i18nd("kdeconnect-kcm", "Available")
                    case (DevicesModel.Reachable | DevicesModel.Paired):
                        return i18nd("kdeconnect-kcm", "Connected")
                }
            }
        }
        Kirigami.PlaceholderMessage {
            text: i18nd("kdeconnect-kcm", "No devices found")
            icon.name: 'edit-none-symbolic'
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)
            visible: devices.count === 0
        }

        delegate: ItemDelegate {
            id: delegate
            icon.name: iconName
            text: model.name
            width: ListView.view.width
            highlighted: root.currentDeviceId === deviceId

            focus: true

            contentItem: Kirigami.IconTitleSubtitle {
                title: delegate.text
                subtitle: toolTip
                icon: icon.fromControlsIcon(delegate.icon)
            }

            onClicked: {
                root.currentDeviceId = deviceId
                root.clicked(deviceId)
            }
        }
    }
}
