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

#include <QTimer>

class PairingHandler : public QObject
{
    Q_OBJECT
protected:

    enum PairStatus {
        NotPaired,
        Requested,
        RequestedByPeer,
        Paired,
    };

    QTimer m_pairingTimeout;
    Device* m_device;
    DeviceLink* m_deviceLink; // We keep the latest link here, if this is destroyed without new link, linkDestroyed is emitted and device will destroy pairing handler
    PairStatus m_pairStatus;

public:
    PairingHandler(Device* device);
    virtual ~PairingHandler() { }

    void setLink(DeviceLink* dl);
    bool isPaired() const { return m_pairStatus == PairStatus::Paired; };
    bool pairRequested() const { return m_pairStatus == PairStatus::Requested; }

    virtual void createPairPackage(NetworkPackage& np) = 0;
    virtual void packageReceived(const NetworkPackage& np) = 0;
    virtual bool requestPairing() = 0;
    virtual bool acceptPairing() = 0;
    virtual void rejectPairing() = 0;
    virtual void unpair() = 0;

public Q_SLOTS:
    void linkDestroyed(QObject*);
    virtual void pairingTimeout() = 0;

private:
    virtual void setAsPaired() = 0;

Q_SIGNALS:
    void pairingDone();
    void unpairingDone();
    void pairingFailed(const QString& error);
    void linkNull();

};


#endif //KDECONNECT_PAIRINGHANDLER_H
