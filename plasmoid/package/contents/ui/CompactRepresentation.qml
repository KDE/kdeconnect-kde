/**
 *  SPDX-FileCopyrightText: 2014-2015 Frederic St-Pierre <me@fredericstpierre.com>
 *  SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 *  SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

DropArea {
    id: root

    required property PlasmoidItem plasmoidItem

    onEntered: drag => {
        if (drag.hasUrls) {
            root.plasmoidItem.expanded = true;
        }
    }

    MouseArea {
        anchors.fill: parent

        property bool wasExpanded: false

        onPressed: mouse => {
            wasExpanded = root.plasmoidItem.expanded;
        }

        onClicked: mouse => {
            root.plasmoidItem.expanded = !root.plasmoidItem.expanded;
        }
    }

    Kirigami.Icon {
        anchors.fill: parent
        source: Plasmoid.icon
    }
}
