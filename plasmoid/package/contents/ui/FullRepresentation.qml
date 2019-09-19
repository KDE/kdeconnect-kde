/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kdeconnect 1.0 as KdeConnect
import QtQuick.Layouts 1.9
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: kdeconnect
    property alias devicesModel: devicesView.model

    KdeConnect.DevicesModel {
        id: allDevicesModel
    }

    ColumnLayout {
        spacing: 5
        visible: devicesView.count == 0
        anchors.fill: parent

        PlasmaExtras.Heading {
            id: heading
            Layout.fillWidth: true
            level: 3
            opacity: 0.6
            text: i18n("No paired devices available")
        }

        Item {
            Layout.fillHeight: true
        }

        PlasmaComponents.Label {
            Layout.fillWidth: true
            Layout.bottomMargin: units.largeSpacing

            visible: allDevicesModel.count == 0

            text: i18n("Install KDE Connect on your Android device to integrate it with Plasma!")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        PlasmaComponents.Button {
            Layout.leftMargin: units.largeSpacing
            Layout.rightMargin: units.largeSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            visible: allDevicesModel.count == 0
            text: i18n("Install from Google Play")
            onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
        }

        PlasmaComponents.Button {
            Layout.leftMargin: units.largeSpacing
            Layout.rightMargin: units.largeSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            visible: allDevicesModel.count == 0
            text: i18n("Install from F-Droid")
            onClicked: Qt.openUrlExternally("https://f-droid.org/en/packages/org.kde.kdeconnect_tp/")
        }

        PlasmaComponents.Button {
            Layout.leftMargin: units.largeSpacing
            Layout.rightMargin: units.largeSpacing
            Layout.topMargin: units.largeSpacing
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: i18n("KDE Connect Settings...")
            onClicked: KCMShell.open("kcm_kdeconnect")
            visible: KCMShell.authorize("kcm_kdeconnect.desktop").length > 0
        }


        Item {
            Layout.fillHeight: true
        }

        Item {
            height: heading.height
        }
    }

    /*
    //Startup arguments
    PlasmaComponents.Label {
        visible: (startupArguments.length > 0)
        text: (""+startupArguments)
        anchors.fill: parent
    }
    */

    PlasmaExtras.ScrollArea {
        id: dialogItem
        anchors.fill: parent
        visible: devicesView.count > 0

        ListView {
            id: devicesView
            anchors.fill: parent
            delegate: DeviceDelegate { }
        }
    }
}
