/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_PAIRINGHANDLER_H
#define KDECONNECT_PAIRINGHANDLER_H

#include "device.h"
#include "networkpacket.h"
#include "pairstate.h"

#include <QTimer>

class KDECONNECTCORE_EXPORT PairingHandler : public QObject
{
    Q_OBJECT
public:
    const static int pairingTimeoutMsec = 30 * 1000; // 30 seconds of timeout

    PairingHandler(Device *parent, PairState initialState);

    void packetReceived(const NetworkPacket &np);

    PairState pairState()
    {
        return m_pairState;
    }

    QString verificationKey() const;

public Q_SLOTS:
    bool requestPairing();
    bool acceptPairing();
    void cancelPairing();
    void unpair();

Q_SIGNALS:
    void incomingPairRequest();
    void pairingFailed(const QString &errorMessage);
    void pairingSuccessful();
    void unpaired();

private:
    void pairingDone();

    QTimer m_pairingTimeout;
    long m_pairingTimestamp;

    Device *m_device;
    PairState m_pairState;

private Q_SLOTS:
    void pairingTimeout();
};

#endif // KDECONNECT_PAIRINGHANDLER_H
