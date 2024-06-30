/**
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.kdeconnect as KDEConnect
import org.kde.kquickcontrolsaddons as KQuickControlsAddons
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    readonly property bool inPanel: [
        PlasmaCore.Types.TopEdge,
        PlasmaCore.Types.RightEdge,
        PlasmaCore.Types.BottomEdge,
        PlasmaCore.Types.LeftEdge,
    ].includes(Plasmoid.location)

    KDEConnect.DevicesModel {
        id: connectedDeviceModel
        displayFilter: KDEConnect.DevicesModel.Paired | KDEConnect.DevicesModel.Reachable
    }

    KDEConnect.DevicesModel {
        id: pairedDeviceModel
        displayFilter: KDEConnect.DevicesModel.Paired
    }

    Plasmoid.icon: inPanel
        ? "kdeconnect-tray-symbolic"
        : "kdeconnect-tray"

    Plasmoid.status: connectedDeviceModel.count > 0 ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus

    fullRepresentation: FullRepresentation {
        devicesModel: connectedDeviceModel
    }

    compactRepresentation: CompactRepresentation {
        plasmoidItem: root
    }

    PlasmaCore.Action {
        id: configureAction
        text: i18n("KDE Connect Settings…")
        icon.name: "configure"
        visible: KConfig.KAuthorized.authorizeControlModule("kcm_kdeconnect")
        onTriggered: checked => {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_kdeconnect");
        }
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }
}
