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
#include <QtCrypto>

#include "core/networkpackage.h"

class PairingHandler;
class NetworkPackage;
class LinkProvider;
class Device;

class DeviceLink
    : public QObject
{
    Q_OBJECT

public:
    enum PairStatus : bool { NotPaired, Paired };
    enum ConnectionStarted : bool { Locally, Remotely };

    DeviceLink(const QString& deviceId, LinkProvider* parent, ConnectionStarted connectionSource);
    virtual ~DeviceLink() { };

    virtual QString name() = 0;

    const QString& deviceId() { return mDeviceId; }
    LinkProvider* provider() { return mLinkProvider; }

    virtual bool sendPackage(NetworkPackage& np) = 0;
    virtual bool sendPackageEncrypted(NetworkPackage& np) = 0;

    //user actions
    virtual void userRequestsPair() = 0;
    virtual void userRequestsUnpair() = 0;

    ConnectionStarted connectionSource() const { return mConnectionSource; }

    PairStatus pairStatus() const { return mPairStatus; }
    void setPairStatus(PairStatus status);

Q_SIGNALS:
    void receivedPackage(const NetworkPackage& np);
    void pairStatusChanged(DeviceLink::PairStatus status);

protected:
    QCA::PrivateKey mPrivateKey;

private:
    const QString mDeviceId;
    const ConnectionStarted mConnectionSource;
    LinkProvider* mLinkProvider;
    PairStatus mPairStatus;

};

#endif
