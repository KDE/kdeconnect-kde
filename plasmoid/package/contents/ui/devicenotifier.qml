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

import QtQuick 1.0
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.kdeconnect 1.0 as KdeConnect

Item {
    id: devicenotifier
    property int implicitWidth: 500
    property int implicitHeight: 500
    property string devicesType: "removable"

    PlasmaCore.Theme {
        id: theme
    }

    PlasmaCore.DataSource {
        id: kdeconnectSource
        engine: "kdeconnect"
        property string last
        onSourceAdded: {
            last = source;
            connectSource(source);
        }
        onDataChanged: {
            if (last != "") {
                statusBar.setData(data[last]["error"], data[last]["errorDetails"], data[last]["udi"]);
                plasmoid.status = "NeedsAttentionStatus";
                plasmoid.showPopup(2500)
            }
            expandDevice(1)
        }
    }




    MouseArea {
        hoverEnabled: true
        anchors.fill: parent

        PlasmaComponents.Label {
            id: header
            text: devicesModel.count>0 ? i18n("Available Devices") : i18n("No Devices Available")
            anchors { top: parent.top; topMargin: 3; left: parent.left; right: parent.right }
            horizontalAlignment: Text.AlignHCenter
        }


        PlasmaCore.Svg {
            id: lineSvg
            imagePath: "widgets/line"
        }
        PlasmaCore.SvgItem {
            id: headerSeparator
            svg: lineSvg
            elementId: "horizontal-line"
            anchors {
                top: header.bottom
                left: parent.left
                right: parent.right
                topMargin: 3
            }
            height: lineSvg.elementSize("horizontal-line").height
        }

        PlasmaExtras.ScrollArea {
            anchors {
                top : headerSeparator.bottom
                topMargin: 10
                bottom: statusBarSeparator.top
                left: parent.left
                right: parent.right
            }
            flickableItem: ListView {
                id: notifierDialog

                model: KdeConnect.DevicesModel {
                    id: devicesModel
                }

                delegate: DeviceDelegate { }

            }
        }

        PlasmaCore.SvgItem {
            id: statusBarSeparator
            svg: lineSvg
            elementId: "horizontal-line"
            height: lineSvg.elementSize("horizontal-line").height
            anchors {
                bottom: statusBar.top
                bottomMargin: statusBar.visible ? 3:0
                left: parent.left
                right: parent.right
            }
            visible: statusBar.height>0
        }

        StatusBar {
            id: statusBar
            anchors {
                left: parent.left
                leftMargin: 5
                right: parent.right
                rightMargin: 5
                bottom: parent.bottom
                bottomMargin: 5
            }
        }
    } // MouseArea


}
