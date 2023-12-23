/**
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.kquickcontrolsaddons
import org.kde.kdeconnect
import org.kde.kcmutils as KCMUtils
import org.kde.config as KConfig

PlasmoidItem
{
    id: root

    readonly property bool inPanel: (plasmoid.location == PlasmaCore.Types.TopEdge
        || plasmoid.location == PlasmaCore.Types.RightEdge
        || plasmoid.location == PlasmaCore.Types.BottomEdge
        || plasmoid.location == PlasmaCore.Types.LeftEdge)

    DevicesModel {
        id: connectDeviceModel
        displayFilter: DevicesModel.Paired | DevicesModel.Reachable
    }

    DevicesModel {
        id: pairedDeviceModel
        displayFilter: DevicesModel.Paired
    }

    Plasmoid.icon: {
        let iconName = "kdeconnect-tray";

        if (inPanel) {
            return "kdeconnect-tray-symbolic";
        }

        return iconName;
    }

    Binding {
        target: plasmoid
        property: "status"
        value: (connectDeviceModel.count > 0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    }

    fullRepresentation: FullRepresentation {
        devicesModel: connectDeviceModel
    }

    compactRepresentation: CompactRepresentation {
    }

    PlasmaCore.Action {
        id: configureAction
        text: i18n("KDE Connect Settings...")
        icon.name: "configure"
        visible: KConfig.KAuthorized.authorizeControlModule("kcm_kdeconnect");
        onTriggered: {
            KCMUtils.KCMLauncher.openSystemSettings("kcm_kdeconnect");
        }
    }

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction);
    }
}
