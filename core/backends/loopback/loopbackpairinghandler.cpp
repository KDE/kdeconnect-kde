/**
 * Copyright 2015 Vineet Garg <grg.vineet@gmail.com>
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

#include <KLocalizedString>

#include "core_debug.h"
#include "daemon.h"
#include "kdeconnectconfig.h"
#include "loopbackdevicelink.h"
#include "loopbackpairinghandler.h"
#include "networkpackagetypes.h"

LoopbackPairingHandler::LoopbackPairingHandler(const QString& deviceId)
    : LanPairingHandler(deviceId)
{

}

bool LoopbackPairingHandler::requestPairing()
{
    switch (pairStatus()) {
        case PairStatus::Paired:
            Q_EMIT pairingFailed(deviceLink()->name().append(" : Already paired").toLatin1().data());
            return false;
        case PairStatus::Requested:
            Q_EMIT pairingFailed(deviceLink()->name().append(" : Pairing already requested for this device").toLatin1().data());
            return false;
        case PairStatus::RequestedByPeer:
            qCDebug(KDECONNECT_CORE) << deviceLink()->name() << " : Pairing already started by the other end, accepting their request.";
            acceptPairing();
            return false;
        case PairStatus::NotPaired:
            ;
    }

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    setPairStatus(PairStatus::Requested);
    m_pairingTimeout.start();
    bool success = deviceLink()->sendPackage(np);
    return success;
}

bool LoopbackPairingHandler::acceptPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    setPairStatus(PairStatus::Paired);
    bool success = deviceLink()->sendPackage(np);
    KdeConnectConfig::instance()->setDeviceProperty(m_deviceId, "certificate", QString::fromLatin1(KdeConnectConfig::instance()->certificate().toPem()));
    return success;
}

