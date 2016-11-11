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
#include "bluetoothpairinghandler.h"
#include "networkpackagetypes.h"

BluetoothPairingHandler::BluetoothPairingHandler(DeviceLink* deviceLink)
    : PairingHandler(deviceLink)
    , m_status(NotPaired)
{
    m_pairingTimeout.setSingleShot(true);
    m_pairingTimeout.setInterval(30 * 1000);  //30 seconds of timeout
    connect(&m_pairingTimeout, &QTimer::timeout, this, &BluetoothPairingHandler::pairingTimeout);
}

void BluetoothPairingHandler::packageReceived(const NetworkPackage& np)
{
    qCDebug(KDECONNECT_CORE) << "Pairing package received!" << np.serialize();

    m_pairingTimeout.stop();

    bool wantsPair = np.get<bool>("pair");

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

            Daemon::instance()->askPairingConfirmation(this);
            setInternalPairStatus(RequestedByPeer);
        }

    } else { //wantsPair == false

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
        case NotPaired:
            ;
    }

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    bool success;
    success = deviceLink()->sendPackage(np);
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
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    bool success = deviceLink()->sendPackage(np);
    if (success) {
        setInternalPairStatus(Paired);
    }
    return success;
}

void BluetoothPairingHandler::rejectPairing()
{
    qCDebug(KDECONNECT_CORE) << "User rejects pairing";
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired);
}

void BluetoothPairingHandler::unpair() {
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired);
}

void BluetoothPairingHandler::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    deviceLink()->sendPackage(np);
    setInternalPairStatus(NotPaired); //Will emit the change as well
    Q_EMIT pairingError(i18n("Timed out"));
}

void BluetoothPairingHandler::setInternalPairStatus(BluetoothPairingHandler::InternalPairStatus status)
{
    m_status = status;
    if (status == Paired) {
        deviceLink()->setPairStatus(DeviceLink::Paired);
    } else {
        deviceLink()->setPairStatus(DeviceLink::NotPaired);
    }
}
