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

#include <kdeconnectconfig.h>
#include "lanpairinghandler.h"
#include "networkpackagetypes.h"
#include "landevicelink.h"

LanPairingHandler::LanPairingHandler(Device* device)
    : PairingHandler(device)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(30 * 1000);  //30 seconds of timeout
    connect(&m_pairingTimeout, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));
}

void LanPairingHandler::createPairPackage(NetworkPackage& np)
{
    np.set("pair", true);
    np.set("publicKey", KdeConnectConfig::instance()->publicKey().toPEM());
}

bool LanPairingHandler::packageReceived(const NetworkPackage& np)
{
    m_pairingTimeout.stop();
    //Retrieve their public key
    QString keyString = np.get<QString>("publicKey");
    m_device->setPublicKey(QCA::RSAPublicKey::fromPEM(keyString));
    if (m_device->publicKey().isNull()) {
        return false;
    }
    return true;
}

bool LanPairingHandler::requestPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    bool success;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        success = dl->sendPackage(np);
    }
    m_pairingTimeout.start();
    return success;
}

bool LanPairingHandler::acceptPairing()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    createPairPackage(np);
    bool success;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
            success = dl->sendPackage(np);
    }
    return success;
}

void LanPairingHandler::rejectPairing()
{
    // TODO : check status of reject pairing
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        dl->sendPackage(np);
    }
}

void LanPairingHandler::pairingDone()
{
    // No need to worry, if either of certificate or public key is null an empty qstring will be returned
    KdeConnectConfig::instance()->setDeviceProperty(m_device->id(), "key", m_device->publicKey().toPEM());
    KdeConnectConfig::instance()->setDeviceProperty(m_device->id(), "certificate", QString(m_device->certificate().toPem()));

    m_pairingTimeout.stop(); // Just in case it is started
}

void LanPairingHandler::unpair() {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        dl->sendPackage(np);
    }
}

void LanPairingHandler::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        dl->sendPackage(np);
    }
    m_device->pairingTimeout();
}
