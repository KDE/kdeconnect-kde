/*
    SPDX-FileCopyrightText: 2014-2015 Frederic St-Pierre <me@fredericstpierre.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

DropArea {
    onEntered: {
        if (drag.hasUrls) {
            root.expanded = true;
        }
    }

    MouseArea {
        id: kdeConnectMouseArea
        anchors.fill: parent

        onClicked: {
            root.expanded = !root.expanded;
        }
    }

    Kirigami.Icon {
        id: kdeConnectIcon
        anchors.fill: parent
        source: plasmoid.icon
    }
}
