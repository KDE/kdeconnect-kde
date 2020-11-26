/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICELINK_H
#define DEVICELINK_H

#include <QObject>

#include "networkpacket.h"

class PairingHandler;
class NetworkPacket;
class LinkProvider;
class Device;
class QSslCertificate;

class DeviceLink
    : public QObject
{
    Q_OBJECT

public:
    enum PairStatus { NotPaired, Paired };

    DeviceLink(const QString& deviceId, LinkProvider* parent);
    virtual ~DeviceLink() = default;

    virtual QString name() = 0;

    const QString& deviceId() const { return m_deviceId; }
    LinkProvider* provider() { return m_linkProvider; }

    virtual bool sendPacket(NetworkPacket& np) = 0;

    //user actions
    virtual void userRequestsPair() = 0;
    virtual void userRequestsUnpair() = 0;

    PairStatus pairStatus() const { return m_pairStatus; }
    virtual void setPairStatus(PairStatus status);

    //The daemon will periodically destroy unpaired links if this returns false
    virtual bool linkShouldBeKeptAlive() { return false; }

    virtual QSslCertificate certificate() const = 0;

Q_SIGNALS:
    void pairingRequest(PairingHandler* handler);
    void pairingRequestExpired(PairingHandler* handler);
    void pairStatusChanged(DeviceLink::PairStatus status);
    void pairingError(const QString& error);
    void receivedPacket(const NetworkPacket& np);

private:
    const QString m_deviceId;
    LinkProvider* m_linkProvider;
    PairStatus m_pairStatus;

};

#endif
