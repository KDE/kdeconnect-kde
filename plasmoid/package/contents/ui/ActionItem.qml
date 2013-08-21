/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2012 Jacopo De Simoi <wilderkde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import Qt 4.7
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.qtextracomponents 0.1
import org.kde.plasma.components 0.1 as PlasmaComponents

Item {
    property string icon
    property alias label: actionText.text
    property string predicate
    height: actionIcon.height+(2*actionsList.actionVerticalMargins)
    width: actionsList.width

    PlasmaCore.IconItem {
        id: actionIcon
        source: QIcon (parent.icon)
        height: actionsList.actionIconHeight
        width: actionsList.actionIconHeight
        anchors {
            top: parent.top
            topMargin: actionsList.actionVerticalMargins
            bottom: parent.bottom
            bottomMargin: actionsList.actionVerticalMargins
            left: parent.left
            leftMargin: 3
        }
    }

    PlasmaComponents.Label {
        id: actionText
        anchors {
            top: actionIcon.top
            bottom: actionIcon.bottom
            left: actionIcon.right
            leftMargin: 5
            right: parent.right
            rightMargin: 3
        }
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            actionsList.currentIndex = index;
            actionsList.highlightItem.opacity = 1;
            makeCurrent();
        }
        onExited: {
            actionsList.highlightItem.opacity = 0;
        }
        onClicked: {
            service = hpSource.serviceForSource(udi);
            operation = service.operationDescription("invokeAction");
            operation.predicate = predicate;
            service.startOperationCall(operation);
            notifierDialog.currentExpanded = -1;
            notifierDialog.currentIndex = -1;
        }
    }
}
