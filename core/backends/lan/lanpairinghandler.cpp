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

LanPairingHandler::LanPairingHandler(Device* device)
    : PairingHandler(device)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(30 * 1000);  //30 seconds of timeout
    connect(&m_pairingTimeout, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));

    if (device->isPaired()) {
        if (!KdeConnectConfig::instance()->getTrustedDevice(device->id()).publicKey.isNull()) {
            m_pairStatus = PairStatus::Paired;
        } else {
            requestPairing(); // Request pairing if device is paired but public key is not there
        }
    } else {
        m_pairStatus = PairStatus::NotPaired;
    }
}

void LanPairingHandler::createPairPackage(NetworkPackage& np)
{
    np.set("link", m_deviceLink->name());
    np.set("pair", true);
    np.set("publicKey", KdeConnectConfig::instance()->publicKey().toPEM());
}

void LanPairingHandler::packageReceived(const NetworkPackage& np)
{

    if (!np.get<QString>("link").compare(m_deviceLink->name())) return; // If this package is not received by my type of link

    m_pairingTimeout.stop();

    bool wantsPair = np.get<bool>("pair");

    if (wantsPair == isPaired()) {
//        qCDebug(KDECONNECT_CORE) << "Already" << (wantsPair? "paired":"unpaired");
        if (m_pairStatus == PairStatus ::Requested) {
            m_pairStatus = PairStatus ::NotPaired;
            Q_EMIT pairingFailed(i18n("Canceled by other peer"));
        } else if (m_pairStatus == PairStatus ::Paired) {
            // Auto accept pairing for the link if device is paired
            acceptPairing();
        }
        return;
    }

    if (wantsPair) {

        QString keyString = np.get<QString>("publicKey");
        m_device->setPublicKey(QCA::RSAPublicKey::fromPEM(keyString));
        bool success = !m_device->publicKey().isNull();
        if (!success) {
            if (m_pairStatus == PairStatus ::Requested) {
                m_pairStatus = PairStatus::NotPaired;
            }
            Q_EMIT pairingFailed(i18n("Received incorrect key"));
            return;
        }

        if (m_pairStatus == PairStatus::Requested)  { //We started pairing

            qCDebug(KDECONNECT_CORE) << "Pair answer";
            setAsPaired();

        } else {
            qCDebug(KDECONNECT_CORE) << "Pair request";

            Daemon::instance()->requestPairing(m_device);

            m_pairStatus = PairStatus ::RequestedByPeer;
        }

    } else {

        qCDebug(KDECONNECT_CORE) << "Unpair request";

        PairStatus prevPairStatus = m_pairStatus;
        m_pairStatus = PairStatus::NotPaired;

        if (prevPairStatus == PairStatus ::Requested) {
            Q_EMIT pairingFailed(i18n("Canceled by other peer"));
        } else if (prevPairStatus == PairStatus::Paired) {
            Q_EMIT (unpairingDone());
        }

    }
}

bool LanPairingHandler::requestPairing()
{
    switch (m_pairStatus) {
        case PairStatus::Paired:
            Q_EMIT pairingFailed(i18n(m_deviceLink->name().append(" : Already paired").toLatin1().data()));
            return false;
        case PairStatus ::Requested:
            Q_EMIT pairingFailed(i18n(m_deviceLink->name().append(" : Pairing already requested for this device").toLatin1().data()));
            return false;
        case PairStatus ::RequestedByPeer:
            qCDebug(KDECONNECT_CORE) << m_deviceLink->name() << " : Pairing already started by the other end, accepting their request.";
            acceptPairing();
            return false;
        case PairStatus::NotPaired:
            ;
    }

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    bool success;
    success = m_deviceLink->sendPackage(np);
    if (success) m_pairStatus = PairStatus::Requested;
    m_pairingTimeout.start();
    return success;
}

bool LanPairingHandler::acceptPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    bool success;
    success = m_deviceLink->sendPackage(np);
    if (success) {
        m_pairStatus = PairStatus::Paired;
        setAsPaired();
        Q_EMIT(pairingDone());
    }
    return success;
}

void LanPairingHandler::rejectPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    m_deviceLink->sendPackage(np);
    m_pairStatus = PairStatus::NotPaired;
}

void LanPairingHandler::unpair() {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    m_deviceLink->sendPackage(np);
    m_pairStatus = PairStatus::NotPaired;

    Q_EMIT(unpairingDone()); // There will be multiple signals if there are multiple pairing handlers
}

void LanPairingHandler::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    m_deviceLink->sendPackage(np);
    m_pairStatus = PairStatus::NotPaired;
    m_device->pairingTimeout(); // Use signal slot, or this is good ?
}

void LanPairingHandler::setAsPaired()
{
    KdeConnectConfig::instance()->setDeviceProperty(m_device->id(), "publicKey", m_device->publicKey().toPEM());
    KdeConnectConfig::instance()->setDeviceProperty(m_device->id(), "certificate", QString::fromLatin1(m_device->certificate().toPem()));

    m_pairStatus = PairStatus::Paired;
    m_pairingTimeout.stop(); // Just in case it is started

    Q_EMIT(pairingDone());
}
