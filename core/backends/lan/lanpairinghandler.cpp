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

LanPairingHandler::LanPairingHandler() {

}

NetworkPackage LanPairingHandler::createPairPackage() {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    np.set("publicKey", KdeConnectConfig::instance()->publicKey().toPEM());
    return np;
}

bool LanPairingHandler::packageReceived(Device *device, NetworkPackage np) {
    //Retrieve their public key
    const QString& keyString = np.get<QString>("publicKey");
    device->setPublicKey(QCA::RSAPublicKey::fromPEM(keyString));
    if (device->publicKey().isNull()) {
        return false;
    }
    return true;
}

bool LanPairingHandler::requestPairing(Device *device) {
    NetworkPackage np = createPairPackage();
    bool success = device->sendPackage(np);
    return success;
}

bool LanPairingHandler::acceptPairing(Device *device) {
    NetworkPackage np = createPairPackage();
    bool success = device->sendPackage(np);
    return success;
}

void LanPairingHandler::rejectPairing(Device *device) {
    // TODO : check status of reject pairing
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    device->sendPackage(np);
}

void LanPairingHandler::pairingDone(Device *device) {
    // TODO : Save certificate and public key here

    // No need to worry, if either of certificate or public key is null an empty qstring will be returned
    KdeConnectConfig::instance()->setDeviceProperty(device->id(), "key", device->publicKey().toPEM());
    KdeConnectConfig::instance()->setDeviceProperty(device->id(), "certificate", QString(device->certificate().toPem()));
}

void LanPairingHandler::unpair(Device *device) {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    bool success = device->sendPackage(np);
}
