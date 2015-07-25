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

#include "device.h"
#include "networkpackage.h"
#include "devicelink.h"

class PairingHandler : public QObject
{
    Q_OBJECT
protected:
    Device* m_device;
    QVector<DeviceLink*> m_deviceLinks;

public:
    PairingHandler(Device* device);
    virtual ~PairingHandler() { }

    void addLink(DeviceLink* dl);
    virtual void createPairPackage(NetworkPackage& np) = 0;
    virtual bool packageReceived(const NetworkPackage& np) = 0;
    virtual bool requestPairing() = 0;
    virtual bool acceptPairing() = 0;
    virtual void rejectPairing() = 0;
    virtual void pairingDone() = 0;
    virtual void unpair() = 0;

public Q_SLOTS:
    void linkDestroyed(QObject*);

Q_SIGNALS:
    void noLinkAvailable();

};


#endif //KDECONNECT_PAIRINGHANDLER_H
