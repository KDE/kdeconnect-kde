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

#ifndef KDECONNECT_PAIRINGHANDLER_H
#define KDECONNECT_PAIRINGHANDLER_H


#include <networkpackage.h>
#include <device.h>

class PairingHandler {

public:
    PairingHandler();
    virtual ~PairingHandler() { }

    virtual NetworkPackage createPairPackage() = 0;
    virtual bool packageReceived(Device *device, NetworkPackage np) = 0;
    virtual bool requestPairing(Device *device) = 0;
    virtual bool acceptPairing(Device *device) = 0;
    virtual void rejectPairing(Device *device) = 0;
    virtual void pairingDone(Device *device) = 0;
    virtual void unpair(Device *device) = 0;

};


#endif //KDECONNECT_PAIRINGHANDLER_H
