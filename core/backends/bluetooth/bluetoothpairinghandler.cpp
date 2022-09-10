/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothpairinghandler.h"

#include <KLocalizedString>

#include "core_debug.h"
#include "daemon.h"
#include "kdeconnectconfig.h"
#include "networkpackettypes.h"

BluetoothPairingHandler::BluetoothPairingHandler(DeviceLink *deviceLink)
    : PairingHandler(deviceLink)
    , m_status(NotPaired)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(pairingTimeoutMsec());
    connect(&m_pairingTimeout, &QTimer::timeout, this, &BluetoothPairingHandler::pairingTimeout);
}

void BluetoothPairingHandler::packetReceived(const NetworkPacket &np)
{
    qCDebug(KDECONNECT_CORE) << "Pairing packet received!" << np.serialize();

    m_pairingTimeout.stop();

    bool wantsPair = np.get<bool>(QStringLiteral("pair"));

    if (wantsPair) {
        if (isPairRequested()) { // We started pairing

            qCDebug(KDECONNECT_CORE) << "Pair answer";
            setInternalPairStatus(Paired);

        } else {
            qCDebug(KDECONNECT_CORE) << "Pair request";

            if (isPaired()) { // I'm already paired, but they think I'm not
                acceptPairing();
                return;
            }

            setInternalPairStatus(RequestedByPeer);
        }

    } else { // wantsPair == false

        qCDebug(KDECONNECT_CORE) << "Unpair request";

        setInternalPairStatus(NotPaired);
        if (isPairRequested()) {
            Q_EMIT pairingError(i18n("Canceled by other peer"));
        }
    }
}

bool BluetoothPairingHandler::requestPairing()
{
    switch (m_status) {
    case Paired:
        Q_EMIT pairingError(i18n("%1: Already paired", deviceLink()->name()));
        return false;
    case Requested:
        Q_EMIT pairingError(i18n("%1: Pairing already requested for this device", deviceLink()->name()));
        return false;
    case RequestedByPeer:
        qCDebug(KDECONNECT_CORE) << deviceLink()->name() << " : Pairing already started by the other end, accepting their request.";
        acceptPairing();
        return false;
    case NotPaired:;
    }

    NetworkPacket np(PACKET_TYPE_PAIR);
    np.set(QStringLiteral("pair"), true);
    bool success;
    success = deviceLink()->sendPacket(np);
    if (success) {
        setInternalPairStatus(Requested);
        m_pairingTimeout.start();
    }
    return success;
}

bool BluetoothPairingHandler::acceptPairing()
{
    qCDebug(KDECONNECT_CORE) << "User accepts pairing";
    m_pairingTimeout.stop(); // Just in case it is started
    NetworkPacket np(PACKET_TYPE_PAIR);
    np.set(QStringLiteral("pair"), true);
    bool success = deviceLink()->sendPacket(np);
    if (success) {
        setInternalPairStatus(Paired);
    }
    return success;
}

void BluetoothPairingHandler::rejectPairing()
{
    qCDebug(KDECONNECT_CORE) << "User rejects pairing";
    NetworkPacket np(PACKET_TYPE_PAIR);
    np.set(QStringLiteral("pair"), false);
    deviceLink()->sendPacket(np);
    setInternalPairStatus(NotPaired);
}

void BluetoothPairingHandler::unpair()
{
    NetworkPacket np(PACKET_TYPE_PAIR);
    np.set(QStringLiteral("pair"), false);
    deviceLink()->sendPacket(np);
    setInternalPairStatus(NotPaired);
}

void BluetoothPairingHandler::pairingTimeout()
{
    NetworkPacket np(PACKET_TYPE_PAIR);
    np.set(QStringLiteral("pair"), false);
    deviceLink()->sendPacket(np);
    setInternalPairStatus(NotPaired); // Will emit the change as well
    Q_EMIT pairingError(i18n("Timed out"));
}

void BluetoothPairingHandler::setInternalPairStatus(BluetoothPairingHandler::InternalPairStatus status)
{
    m_status = status;
    if (status == Paired) {
        deviceLink()->setPairStatus(DeviceLink::Paired);
    } else if (status == NotPaired) {
        deviceLink()->setPairStatus(DeviceLink::NotPaired);
    } else if (status == RequestedByPeer) {
        Q_EMIT deviceLink()->pairingRequest(this);
    }
}
