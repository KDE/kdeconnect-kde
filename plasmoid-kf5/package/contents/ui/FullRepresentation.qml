/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.4
import QtQuick.Controls 2.4
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kdeconnect 1.0 as KdeConnect
import QtQuick.Layouts 1.9
import org.kde.kquickcontrolsaddons 2.0

PlasmaExtras.Representation {
    id: kdeconnect
    property alias devicesModel: devicesView.model
    collapseMarginsHint: true

    KdeConnect.DevicesModel {
        id: allDevicesModel
    }
    KdeConnect.DevicesModel {
        id: pairedDevicesModel
        displayFilter: KdeConnect.DevicesModel.Paired
    }

    PlasmaComponents3.ScrollView {
        id: dialogItem
        anchors.fill: parent

        contentItem: ListView {
            id: devicesView
            spacing: PlasmaCore.Units.smallSpacing

            delegate: DeviceDelegate { }

            PlasmaExtras.PlaceholderMessage {
                width: parent.width - PlasmaCore.Units.gridUnit * 2
                anchors.centerIn: parent
                visible: devicesView.count === 0

                iconName: {
                    if (pairedDevicesModel.count >= 0) {
                        return pairedDevicesModel.count === 0 ? "edit-none" : "network-disconnect";
                    }
                    return "kdeconnect";
                }

                text: {
                    if (pairedDevicesModel.count >= 0) {
                        return pairedDevicesModel.count == 0 ? i18n("No paired devices") : i18np("Paired device is unavailable", "All paired devices are unavailable", pairedDevicesModel.count)
                    } else if (allDevicesModel.count == 0) {
                        return i18n("Install KDE Connect on your Android device to integrate it with Plasma!")
                    }
                }
                helpfulAction: Action {
                    text: i18n("Pair a Device...")
                    icon.name: "list-add"
                    onTriggered: KCMShell.openSystemSettings("kcm_kdeconnect")
                    enabled: pairedDevicesModel.count == 0 && KCMShell.authorize("kcm_kdeconnect.desktop").length > 0
                }

                PlasmaComponents3.Button {
                    Layout.leftMargin: units.largeSpacing * 3
                    Layout.rightMargin: units.largeSpacing * 3
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    visible: allDevicesModel.count === 0
                    text: i18n("Install from Google Play")
                    onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
                }

                PlasmaComponents3.Button {
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
