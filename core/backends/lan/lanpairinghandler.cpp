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
#include "landevicelink.h"
#include "lanpairinghandler.h"
#include "networkpackagetypes.h"

LanPairingHandler::LanPairingHandler(DeviceLink* deviceLink)
    : PairingHandler(deviceLink)
    , m_status(NotPaired)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(pairingTimeoutMsec());
    connect(&m_pairingTimeout, &QTimer::timeout, this, &LanPairingHandler::pairingTimeout);
}

void LanPairingHandler::packageReceived(const NetworkPackage& np)
{
    bool wantsPair = np.get<bool>(QStringLiteral("pair"));

    if (wantsPair) {

        if (isPairRequested())  { //We started pairing

            qCDebug(KDECONNECT_CORE) << "Pair answer";
            setInternalPairStatus(Paired);
            
        } else {
            qCDebug(KDECONNECT_CORE) << "Pair request";

            if (isPaired()) { //I'm already paired, but they think I'm not
                acceptPairing();
                return;
            }

            setInternalPairStatus(RequestedByPeer);
        }

    } else { //wantsPair == false

        qCDebug(KDECONNECT_CORE) << "Unpair request";

         if (isPairRequested()) {
            Q_EMIT pairingError(i18n("Canceled by other peer"));
        }
        setInternalPairStatus(NotPaired);
    }
}

bool LanPairingHandler::requestPairing()
{
    if (m_status == Paired) {
        Q_EMIT pairingError(i18n("%1: Already paired", deviceLink()->name()));
        return false;
    }
    if (m_status == RequestedByPeer) {
        qCDebug(KDECONNECT_CORE) << deviceLink()->name() << ": Pairing already started by the other end, accepting their request.";
        return acceptPairing();
    }

    NetworkPackage np(PACKAGE_TYPE_PAIR, {{"pair", true}});
    const bool success = deviceLink()->sendPackage(np);
    if (success) {
        setInternalPairStatus(Requested);
    }
    return success;
}

bool LanPairingHandler::acceptPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR, {{"pair", true}});
    bool success = deviceLink()->sendPackage(np);
    if (success) {
        setInternalPairStatus(Paired);
    }
    return success;
}

void LanPairingHandler::rejectPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR, {{"pair", false}});
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired);
}

void LanPairingHandler::unpair() {
    NetworkPackage np(PACKAGE_TYPE_PAIR, {{"pair", false}});
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired);
}

void LanPairingHandler::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR, {{"pair", false}});
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired); //Will emit the change as well
    Q_EMIT pairingError(i18n("Timed out"));
}

void LanPairingHandler::setInternalPairStatus(LanPairingHandler::InternalPairStatus status)
{
    if (status == Requested || status == RequestedByPeer) {
        m_pairingTimeout.start();
    } else {
        m_pairingTimeout.stop();
    }

    if (m_status == RequestedByPeer && (status == NotPaired || status == Paired)) {
        Q_EMIT deviceLink()->pairingRequestExpired(this);
    } else if (status == RequestedByPeer) {
        Q_EMIT deviceLink()->pairingRequest(this);
    }

    m_status = status;
    if (status == Paired) {
        deviceLink()->setPairStatus(DeviceLink::Paired);
    } else {
        deviceLink()->setPairStatus(DeviceLink::NotPaired);
    }
}
