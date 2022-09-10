/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_LANPAIRINGHANDLER_H
#define KDECONNECT_LANPAIRINGHANDLER_H

#include <QObject>
#include <QTimer>

#include "backends/devicelink.h"
#include "backends/pairinghandler.h"
#include "device.h"

// This class is used pairing related stuff. It has direct access to links and can directly send packets
class LanPairingHandler : public PairingHandler
{
    Q_OBJECT

public:
    enum InternalPairStatus {
        NotPaired,
        Requested,
        RequestedByPeer,
        Paired,
    };

    LanPairingHandler(DeviceLink *deviceLink);
    ~LanPairingHandler() override
    {
    }

    void packetReceived(const NetworkPacket &np) override;
    bool requestPairing() override;
    bool acceptPairing() override;
    void rejectPairing() override;
    void unpair() override;

    bool isPairRequested() const
    {
        return m_status == Requested;
    }
    bool isPaired() const
    {
        return m_status == Paired;
    }

private Q_SLOTS:
    void pairingTimeout();

protected:
    void setInternalPairStatus(InternalPairStatus status);

    QTimer m_pairingTimeout;

    InternalPairStatus m_status;
};

#endif // KDECONNECT_LANPAIRINGHANDLER_H
