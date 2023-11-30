/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kdeconnect as KdeConnect
import QtQuick.Layouts
import org.kde.kquickcontrolsaddons
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils
import org.kde.config as KConfig

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
            spacing: Kirigami.Units.smallSpacing

            delegate: DeviceDelegate {
                width: parent.width
            }

            PlasmaExtras.PlaceholderMessage {
                width: parent.width - Kirigami.Units.gridUnit * 2
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
                    onTriggered: KCMUtils.KCMLauncher.openSystemSettings("kcm_kdeconnect")
                    enabled: pairedDevicesModel.count == 0 && KConfig.KAuthorized.authorizeControlModule("kcm_kdeconnect")
                }

                PlasmaComponents3.Button {
                    Layout.leftMargin: Kirigami.Units.gridUnit * 3
                    Layout.rightMargin: Kirigami.Units.gridUnit * 3
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    visible: allDevicesModel.count === 0
                    text: i18n("Install from Google Play")
                    onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
                }

                PlasmaComponents3.Button {
                    Layout.leftMargin: Kirigami.Units.gridUnit * 3
                    Layout.rightMargin: Kirigami.Units.gridUnit * 3
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
