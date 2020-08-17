/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_PAIRINGHANDLER_H
#define KDECONNECT_PAIRINGHANDLER_H

#include "networkpacket.h"
#include "devicelink.h"

/*
 * This class separates the pairing interface for each type of link.
 * Since different links can pair via different methods, like for LanLink certificate and public key should be shared,
 * for Bluetooth link they should be paired via bluetooth etc.
 * Each "Device" instance maintains a hash map for these pairing handlers so that there can be single pairing handler per
 * per link type per device.
 * Pairing handler keeps information about device, latest link, and pair status of the link
 * During first pairing process, the pairing process is nearly same as old process.
 * After that if any one of the link is paired, then we can say that device is paired, so new link will pair automatically
 */

class KDECONNECTCORE_EXPORT PairingHandler
    : public QObject
{
    Q_OBJECT

public:
    PairingHandler(DeviceLink* parent);
    ~PairingHandler() override = default;

    DeviceLink* deviceLink() const;
    void setDeviceLink(DeviceLink* dl);

    virtual void packetReceived(const NetworkPacket& np) = 0;
    virtual void unpair() = 0;
    static int pairingTimeoutMsec() { return 30 * 1000; } // 30 seconds of timeout (default), subclasses that use different values should override

public Q_SLOTS:
    virtual bool requestPairing() = 0;
    virtual bool acceptPairing() = 0;
    virtual void rejectPairing() = 0;

Q_SIGNALS:
    void pairingError(const QString& errorMessage);

private:
    DeviceLink* m_deviceLink;

};


#endif //KDECONNECT_PAIRINGHANDLER_H
