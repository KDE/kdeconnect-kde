/**
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kdeconnect 1.0

Item
{
    DevicesModel {
        id: connectDeviceModel
        displayFilter: DevicesModel.Paired | DevicesModel.Reachable
    }

    DevicesModel {
        id: pairedDeviceModel
        displayFilter: DevicesModel.Paired
    }

    Binding {
        target: plasmoid
        property: "status"
        value: (connectDeviceModel.count > 0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    }

    Plasmoid.fullRepresentation: FullRepresentation {
        devicesModel: connectDeviceModel
    }

    Plasmoid.compactRepresentation: CompactRepresentation {
    }

    readonly property bool isConstrained: (plasmoid.formFactor == PlasmaCore.Types.Vertical || plasmoid.formFactor == PlasmaCore.Types.Horizontal)

    Plasmoid.preferredRepresentation: isConstrained ? Plasmoid.compactRepresentation : Plasmoid.fullRepresentation

    function action_launchkcm() {
        KCMShell.openSystemSettings("kcm_kdeconnect");
    }

    Component.onCompleted: {
        plasmoid.removeAction("configure");
        if (KCMShell.authorize("kcm_kdeconnect.desktop").length > 0) {
            plasmoid.setAction("launchkcm", i18n("KDE Connect Settings..."), "configure");
        }
    }
}
