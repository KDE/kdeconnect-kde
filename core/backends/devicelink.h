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

class DeviceLink : public QObject
{
    Q_OBJECT

public:

    DeviceLink(const QString &deviceId, LinkProvider *parent);
    ~DeviceLink() override = default;

    const QString &deviceId() const
    {
        return m_deviceId;
    }
    LinkProvider *provider()
    {
        return m_linkProvider;
    }

    virtual bool sendPacket(NetworkPacket &np) = 0;

    virtual QSslCertificate certificate() const = 0;

Q_SIGNALS:
    void receivedPacket(const NetworkPacket &np);

private:
    const QString m_deviceId;
    LinkProvider *m_linkProvider;
};

#endif
