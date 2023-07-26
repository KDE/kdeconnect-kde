/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pairinghandler.h"

#include "core_debug.h"

#include <KLocalizedString>

PairingHandler::PairingHandler(Device *parent, PairState initialState)
    : QObject(parent)
    , m_device(parent)
    , m_pairState(initialState)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(pairingTimeoutMsec);
    connect(&m_pairingTimeout, &QTimer::timeout, this, &PairingHandler::pairingTimeout);
}

void PairingHandler::packetReceived(const NetworkPacket &np)
{
    m_pairingTimeout.stop();
    bool wantsPair = np.get<bool>(QStringLiteral("pair"));
    if (wantsPair) {
        switch (m_pairState) {
        case PairState::Requested:
            pairingDone();
            break;
        case PairState::RequestedByPeer:
            qCDebug(KDECONNECT_CORE) << "Ignoring second pairing request before the first one timed out";
            break;
        case PairState::Paired:
        case PairState::NotPaired:
            if (m_pairState == PairState::Paired) {
                qWarning() << "Received pairing request from a device we already trusted.";
                // It would be nice to auto-accept the pairing request here, but since the pairing accept and pairing request
                // messages are identical, this could create an infinite loop if both devices are "accepting" each other pairs.
                // Instead, unpair and handle as if "NotPaired".
                m_pairState = PairState::NotPaired;
                Q_EMIT unpaired();
            }
            m_pairState = PairState::RequestedByPeer;
            m_pairingTimeout.start();
            Q_EMIT incomingPairRequest();
            break;
        }
    } else { // wantsPair == false
        qCDebug(KDECONNECT_CORE) << "Unpair request received";
        switch (m_pairState) {
        case PairState::NotPaired:
            qCDebug(KDECONNECT_CORE) << "Ignoring unpair request for already unpaired device";
            break;
        case PairState::Requested: // We started pairing and got rejected
        case PairState::RequestedByPeer: // They stared pairing, then cancelled
            m_pairState = PairState::NotPaired;
            Q_EMIT pairingFailed(i18n("Canceled by other peer"));
            break;
        case PairState::Paired:
            m_pairState = PairState::NotPaired;
            Q_EMIT unpaired();
            break;
        }
    }
}

bool PairingHandler::requestPairing()
{
    m_pairingTimeout.stop();

    if (m_pairState == PairState::Paired) {
        qWarning() << m_device->name() << ": requestPairing was called on an already paired device.";
        Q_EMIT pairingFailed(i18n("%1: Already paired", m_device->name()));
        return false;
    }
    if (m_pairState == PairState::RequestedByPeer) {
        qWarning() << m_device->name() << ": Pairing already started by the other end, accepting their request.";
        return acceptPairing();
    }

    if (!m_device->isReachable()) {
        Q_EMIT pairingFailed(i18n("%1: Device not reachable", m_device->name()));
        return false;
    }

    m_pairState = PairState::Requested;

    m_pairingTimeout.start();

    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), true}});
    const bool success = m_device->sendPacket(np);
    if (!success) {
        m_pairingTimeout.stop();
        qWarning() << m_device->name() << ": Failed to send pair request packet.";
        m_pairState = PairState::NotPaired;
        Q_EMIT pairingFailed(i18n("%1: Device not reachable", m_device->name()));
    }
    return success;
}

bool PairingHandler::acceptPairing()
{
    m_pairingTimeout.stop();
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), true}});
    const bool success = m_device->sendPacket(np);
    if (success) {
        pairingDone();
    } else {
        qWarning() << "Failed to send packet accepting pairing";
        m_pairState = PairState::NotPaired;
        Q_EMIT pairingFailed(i18n("Device not reachable"));
    }
    return success;
}

void PairingHandler::cancelPairing()
{
    m_pairingTimeout.stop();
    m_pairState = PairState::NotPaired;
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    m_device->sendPacket(np);
    Q_EMIT pairingFailed(i18n("Cancelled by user"));
}

void PairingHandler::unpair()
{
    m_pairState = PairState::NotPaired;
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    m_device->sendPacket(np);
    Q_EMIT unpaired();
}

void PairingHandler::pairingTimeout()
{
    NetworkPacket np(PACKET_TYPE_PAIR, {{QStringLiteral("pair"), false}});
    m_device->sendPacket(np);
    m_pairState = PairState::NotPaired;
    Q_EMIT pairingFailed(i18n("Timed out"));
}

void PairingHandler::pairingDone()
{
    qCDebug(KDECONNECT_CORE) << "Pairing done";
    m_pairState = PairState::Paired;
    Q_EMIT pairingSuccessful();
}

#include "moc_pairinghandler.cpp"
