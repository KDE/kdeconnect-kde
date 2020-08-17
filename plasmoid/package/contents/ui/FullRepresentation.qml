/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.4
import QtQuick.Controls 2.4
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kdeconnect 1.0 as KdeConnect
import QtQuick.Layouts 1.9
import org.kde.kquickcontrolsaddons 2.0

import org.kde.kirigami 2.12 as Kirigami

Item {
    id: kdeconnect
    property alias devicesModel: devicesView.model

    KdeConnect.DevicesModel {
        id: allDevicesModel
    }
    KdeConnect.DevicesModel {
        id: pairedDevicesModel
        displayFilter: KdeConnect.DevicesModel.Paired
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

        ListView {
            id: devicesView
            anchors.fill: parent
            delegate: DeviceDelegate { }

            Kirigami.PlaceholderMessage {
                // For optimal label and button sizing
                width: units.gridUnit * 20
                anchors.centerIn: parent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: units.largeSpacing

                visible: devicesView.count == 0

                text: {
                    if (pairedDevicesModel.count >= 0) {
                        return pairedDevicesModel.count == 0 ? i18n("No paired devices") : i18np("Paired device is unavailable", "All paired devices are unavailable", pairedDevicesModel.count)
                    } else if (allDevicesModel.count == 0) {
                        return i18n("Install KDE Connect on your Android device to integrate it with Plasma!")
                    }
                }
                helpfulAction: Action {
                    text: pairedDevicesModel.count == 0 ? i18n("Pair a Device...") : i18n("Configure...")
                    icon.name: pairedDevicesModel.count == 0 ? "list-add" : "configure"
                    onTriggered: KCMShell.open("kcm_kdeconnect")
                    enabled: KCMShell.authorize("kcm_kdeconnect.desktop").length > 0
                }

                PlasmaComponents.Button {
                    Layout.leftMargin: units.largeSpacing * 3
                    Layout.rightMargin: units.largeSpacing * 3
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    visible: allDevicesModel.count === 0
                    text: i18n("Install from Google Play")
                    onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
                }

                PlasmaComponents.Button {
                    Layout.leftMargin: units.largeSpacing * 3
                    Layout.rightMargin: units.largeSpacing * 3
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    visible: allDevicesModel.count === 0
                    text: i18n("Install from F-Droid")
                    onClicked: Qt.openUrlExternally("https://f-droid.org/en/packages/org.kde.kdeconnect_tp/")
                }
            }
        }
    }
}
