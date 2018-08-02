/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef DEVICELINK_H
#define DEVICELINK_H

#include <QObject>
#include <QIODevice> //Fix build on older QCA
#include <QtCrypto>

#include "core/networkpacket.h"

class PairingHandler;
class NetworkPacket;
class LinkProvider;
class Device;

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

Q_SIGNALS:
    void pairingRequest(PairingHandler* handler);
    void pairingRequestExpired(PairingHandler* handler);
    void pairStatusChanged(DeviceLink::PairStatus status);
    void pairingError(const QString& error);
    void receivedPacket(const NetworkPacket& np);

protected:
    QCA::PrivateKey m_privateKey;

private:
    const QString m_deviceId;
    LinkProvider* m_linkProvider;
    PairStatus m_pairStatus;

};

#endif
