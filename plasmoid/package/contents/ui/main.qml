/**
 * Copyright 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
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

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kdeconnect 1.0

Item
{
    width: units.gridUnit * 20
    height: units.gridUnit * 32

    DevicesModel {
        id: connectDeviceModel
        displayFilter: DevicesModel.Paired | DevicesModel.Reachable

    }

    Binding {
        target: plasmoid
        property: "status"
        value: (connectDeviceModel.count > 0) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    }

    Plasmoid.compactRepresentation: CompactRepresentation {}
    Plasmoid.fullRepresentation: FullRepresentation {
        devicesModel: connectDeviceModel
    }

    readonly property bool isConstrained: (plasmoid.formFactor == PlasmaCore.Types.Vertical || plasmoid.formFactor == PlasmaCore.Types.Horizontal)

    Plasmoid.preferredRepresentation: isConstrained ? Plasmoid.compactRepresentation : Plasmoid.fullRepresentation

    function action_launchkcm() {
        KCMShell.open("kcm_kdeconnect");
    }

    Component.onCompleted: {
        plasmoid.removeAction("configure");
        if (KCMShell.authorize("kcm_kdeconnect.desktop").length > 0) {
            plasmoid.setAction("launchkcm", i18n("KDE Connect Settings..."), "configure");
        }
    }
}
