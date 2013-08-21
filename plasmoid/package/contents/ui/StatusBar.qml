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
import org.kde.plasma.components 0.1 as PlasmaComponents

Item {
    id: statusBar
    property bool expanded: false
    visible: false
    height: visible ? (expanded ? statusText.paintedHeight+detailsText.paintedHeight+3 : statusText.paintedHeight) : 0

    Behavior on height { NumberAnimation { duration: 200 } }

    function setData(error, details, udi) {
        shown = visible;
        if (shown)
            close();
        statusText.text = error;
        detailsText.text = details;
        if (shown)
            showTimer.restart();
        else
            show();
    }

    function show() {
        expanded = false;
        visible = true;
        showTimer.stop();
        hideTimer.restart();
    }

    function toggleDetails() {
        expanded = !expanded;
        hideTimer.running = !expanded;
    }

    function close() {
        hideTimer.stop();
        showTimer.stop();
        expanded = false;
        visible = false;
    }

    Timer {
        id: hideTimer
        interval: 7500
        onTriggered: close();
    }

    Timer {
        id: showTimer
        interval: 300
        onTriggered: show();
    }

    PlasmaCore.Svg {
        id: iconsSvg
        imagePath: "widgets/configuration-icons"
    }

    property int iconSize: 16

    PlasmaCore.SvgItem {
        id: closeBtn
        width: iconSize
        height: iconSize
        svg: iconsSvg
        elementId: "close"
        anchors {
            top: parent.top
            right: parent.right
        }
    }

    MouseArea {
        anchors.fill: closeBtn
        onClicked: close();
    }

    PlasmaCore.SvgItem {
        id: detailsBtn
        visible: detailsText.text!=""
        width: visible ? iconSize : 0
        height: visible ? iconSize : 0
        svg: iconsSvg
        elementId: expanded ? "collapse" : "restore"
        anchors {
            top: parent.top
            right: closeBtn.left
            rightMargin: 5
        }
    }

    MouseArea {
        anchors.fill: detailsBtn
        enabled: detailsBtn.visible
        onClicked: toggleDetails();
    }

    PlasmaComponents.Label {
        id: statusText
        anchors {
            top: parent.top
            left: parent.left
            right: detailsBtn.left
            bottom: detailsText.top
            bottomMargin: expanded ? 3 : 0
        }
        clip: true
        wrapMode: Text.WordWrap
    }

    PlasmaComponents.Label {
        id: detailsText
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom 
        }
        font.pointSize: theme.defaultFont.pointSize * 0.8
        clip: true
        wrapMode: Text.WordWrap
        height: expanded ? paintedHeight : 0

        Behavior on height { NumberAnimation { duration: 200 } }
    }
}
