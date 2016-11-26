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

#ifndef KDECONNECT_LANPAIRINGHANDLER_H
#define KDECONNECT_LANPAIRINGHANDLER_H

#include <QObject>
#include <QTimer>

#include "device.h"
#include "backends/devicelink.h"
#include "backends/pairinghandler.h"

// This class is used pairing related stuff. It has direct access to links and can directly send packages
class LanPairingHandler
    : public PairingHandler
{
    Q_OBJECT

public:

    enum InternalPairStatus {
        NotPaired,
        Requested,
        RequestedByPeer,
        Paired,
    };

    LanPairingHandler(DeviceLink* deviceLink);
    ~LanPairingHandler() override { }

    void packageReceived(const NetworkPackage& np) override;
    bool requestPairing() override;
    bool acceptPairing() override;
    void rejectPairing() override;
    void unpair() override;

    bool isPairRequested() const { return m_status == Requested; }
    bool isPaired() const { return m_status == Paired; }

private Q_SLOTS:
    void pairingTimeout();

protected:
    void setInternalPairStatus(InternalPairStatus status);

    QTimer m_pairingTimeout;

    InternalPairStatus m_status;
};


#endif //KDECONNECT_LANPAIRINGHANDLER_H
