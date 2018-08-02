/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.kde.kdeconnect 1.0

Page
{
    id: mousepad
    property QtObject pluginInterface
    backNavigation: false

    Column
    {
        anchors.fill: parent
        PageHeader {
            id: header
            title: "Remote Control"
        }
        MouseArea {
            id: area
            width: parent.width
            height: parent.height - buttons.height - header.height - 20
            property var lastPos: Qt.point(-1, -1)

            //onClicked: mousepad.pluginInterface.sendCommand("singleclick", true);

            onPositionChanged: {
                if (lastPos.x > -1) {
                    //console.log("move", mouse.x, mouse.y, lastPos)
                    var delta = Qt.point(mouse.x-lastPos.x, mouse.y-lastPos.y);

                    pluginInterface.moveCursor(delta);
                }
                lastPos = Qt.point(mouse.x, mouse.y);
            }
            onReleased: {
                lastPos = Qt.point(-1, -1)
            }
        }
        Row {
            id: buttons
            height: childrenRect.height
            width: parent.width

            Button {
                width: parent.width / 3
                text: "Single"
                onClicked: mousepad.pluginInterface.sendCommand("singleclick", true);
            }
            Button {
                width: parent.width / 3
                text: "Middle"
                onClicked: mousepad.pluginInterface.sendCommand("middleclick", true);
            }
            Button {
                width: parent.width / 3
                text: "Right"
                onClicked: mousepad.pluginInterface.sendCommand("rightclick", true);
            }
        }
    }

   function myPop() {
        pageStack._pageStackIndicator._backPageIndicator().data[0].clicked.disconnect(myPop)
        pageStack.pop()
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            pageStack._createPageIndicator()
            pageStack._pageStackIndicator.clickablePageIndicators = true
            pageStack._pageStackIndicator._backPageIndicator().backNavigation = true
            pageStack._pageStackIndicator._backPageIndicator().data[0].clicked.connect(myPop)
        } else if (status == PageStatus.Deactivating) {
            pageStack._pageStackIndicator.clickablePageIndicators = Qt.binding(function() {
                return pageStack.currentPage ? pageStack.currentPage._clickablePageIndicators : true
            })
            pageStack._pageStackIndicator._backPageIndicator().backNavigation = Qt.binding(function() {
                return pageStack._currentContainer && pageStack._currentContainer.page
                       && pageStack._currentContainer.page.backNavigation && pageStack._currentContainer.pageStackIndex !== 0
            })
        }
}

}
