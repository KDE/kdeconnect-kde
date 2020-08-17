/*
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ScrollablePage
{
    id: root
    title: i18nd("kdeconnect-app", "Run command")
    property QtObject pluginInterface

    actions.main: Kirigami.Action {
        icon.name: "document-edit"
        text: i18nd("kdeconnect-app", "Edit commands")
        onTriggered: {
            pluginInterface.editCommands();
            showPassiveNotification(i18nd("kdeconnect-app", "You can edit commands on the connected device"));
        }
    }

    ListView {
        id: commandsList
        model: RemoteCommandsModel {
            deviceId: pluginInterface.deviceId
        }
        delegate: Kirigami.BasicListItem {
            width: ListView.view.width
            label: name
            subtitle: command
            onClicked: pluginInterface.triggerCommand(key)
            reserveSpaceForIcon: false
        }

        Kirigami.PlaceholderMessage {
            visible: commandsList.count === 0
            text: i18nd("kdeconnect-app", "No commands defined")
            anchors.centerIn: parent
        }
    }
}

