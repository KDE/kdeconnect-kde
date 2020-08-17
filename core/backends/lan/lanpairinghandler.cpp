/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "lanpairinghandler.h"

#include <KLocalizedString>

#include "core_debug.h"
#include "daemon.h"
#include "kdeconnectconfig.h"
#include "landevicelink.h"
#include "networkpackettypes.h"

LanPairingHandler::LanPairingHandler(DeviceLink* deviceLink)
    : PairingHandler(deviceLink)
    , m_status(NotPaired)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(pairingTimeoutMsec());
    connect(&m_pairingTimeout, &QTimer::timeout, this, &LanPairingHandler::pairingTimeout);
}

void LanPairingHandler::packetReceived(const NetworkPacket& np)
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

    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), true}});
    const bool success = deviceLink()->sendPacket(np);
    if (success) {
        setInternalPairStatus(Requested);
    }
    return success;
}

bool LanPairingHandler::acceptPairing()
{
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), true}});
    bool success = deviceLink()->sendPacket(np);
    if (success) {
        setInternalPairStatus(Paired);
    }
    return success;
}

void LanPairingHandler::rejectPairing()
{
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    deviceLink()->sendPacket(np);
    setInternalPairStatus(NotPaired);
}

void LanPairingHandler::unpair() {
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    deviceLink()->sendPacket(np);
    setInternalPairStatus(NotPaired);
}

void LanPairingHandler::pairingTimeout()
{
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    deviceLink()->sendPacket(np);
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
