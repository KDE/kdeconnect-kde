/*
 * Copyright 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

