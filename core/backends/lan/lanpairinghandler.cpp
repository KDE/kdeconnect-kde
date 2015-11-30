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

LanPairingHandler::LanPairingHandler(const QString& deviceId)
    : PairingHandler()
    , m_deviceId(deviceId)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(30 * 1000);  //30 seconds of timeout
    connect(&m_pairingTimeout, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));
/*
    m_publicKey = KdeConnectConfig::instance()->getDeviceProperty(deviceId, "publicKey", QString());
    m_certificate = KdeConnectConfig::instance()->getDeviceProperty(deviceId, "certificate", QString());
*/
    if (!m_publicKey.isNull()) {
        setPairStatus(PairStatus::Paired);
    } else {
        setPairStatus(PairStatus::NotPaired);
    }
}

void LanPairingHandler::createPairPackage(NetworkPackage& np)
{
    np.set("link", deviceLink()->name());
    np.set("pair", true);
    np.set("publicKey", KdeConnectConfig::instance()->publicKey().toPEM());
}

void LanPairingHandler::packageReceived(const NetworkPackage& np)
{

    if (np.get<QString>("link", deviceLink()->name()).compare(deviceLink()->name()) != 0) return; // If this package is not received by my type of link

    m_pairingTimeout.stop();

    bool wantsPair = np.get<bool>("pair");

    if (wantsPair == isPaired()) {
//        qCDebug(KDECONNECT_CORE) << "Already" << (wantsPair? "paired":"unpaired");
        if (isPairRequested()) {
            setPairStatus(PairStatus::NotPaired);
            Q_EMIT pairingFailed(i18n("Canceled by other peer"));
            return;
        } else if (isPaired()) {
            /**
             * If wants pair is true and is paired is true, this means other device is trying to pair again, might be because it unpaired this device somehow
             * and we don't know it, unpair it internally
             */
            KdeConnectConfig::instance()->removeTrustedDevice(m_deviceId);
            setPairStatus(PairingHandler::NotPaired);
        }
    }

    if (wantsPair) {

        QString keyString = np.get<QString>("publicKey");
        QString certificateString = np.get<QByteArray>("certificate");

        if (QCA::RSAPublicKey::fromPEM(keyString).isNull()) {
            if (isPairRequested()) {
                setPairStatus(PairStatus::NotPaired);
            }
            Q_EMIT pairingFailed(i18n("Received incorrect key"));
            return;
        }

        if (isPairRequested())  { //We started pairing

            qCDebug(KDECONNECT_CORE) << "Pair answer";
            KdeConnectConfig::instance()->setDeviceProperty(m_deviceId, "publicKey", keyString);
            KdeConnectConfig::instance()->setDeviceProperty(m_deviceId, "certificate", certificateString);
            
        } else {
            qCDebug(KDECONNECT_CORE) << "Pair request";

            if (isPaired()) {
                acceptPairing();
                return;
            }

            //Daemon::instance()->requestPairing(m_device);
            //m_pairStatus = PairStatus::RequestedByPeer;
            setPairStatus(PairStatus::RequestedByPeer);
        }

    } else { //wantsPair == false

        qCDebug(KDECONNECT_CORE) << "Unpair request";

        if (isPairRequested()) {
            Q_EMIT pairingFailed(i18n("Canceled by other peer"));
        }

        setPairStatus(PairStatus::NotPaired);
    }
}

bool LanPairingHandler::requestPairing()
{
    switch (pairStatus()) {
        case PairStatus::Paired:
            Q_EMIT pairingFailed(i18n(deviceLink()->name().append(" : Already paired").toLatin1().data()));
            return false;
        case PairStatus::Requested:
            Q_EMIT pairingFailed(i18n(deviceLink()->name().append(" : Pairing already requested for this device").toLatin1().data()));
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
    bool success;
    success = deviceLink()->sendPackage(np);
    if (success) {
        setPairStatus(PairStatus::Requested);
        m_pairingTimeout.start();
    }
    return success;
}

bool LanPairingHandler::acceptPairing()
{
    m_pairingTimeout.stop(); // Just in case it is started
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    bool success;
    success = deviceLink()->sendPackage(np);
    if (success) {
        setPairStatus(PairStatus::Paired);
    }
    return success;
}

void LanPairingHandler::rejectPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    np.set("link", deviceLink()->name());
    deviceLink()->sendPackage(np);
    setPairStatus(PairStatus::NotPaired);
}

void LanPairingHandler::unpair() {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    np.set("link", deviceLink()->name());
    deviceLink()->sendPackage(np);
    setPairStatus(PairStatus::NotPaired);
}

void LanPairingHandler::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    np.set("name", deviceLink()->name());
    deviceLink()->sendPackage(np);
    setPairStatus(PairStatus::NotPaired); //Will emit the change as well
    Q_EMIT pairingFailed(i18n("Timed out"));
}
