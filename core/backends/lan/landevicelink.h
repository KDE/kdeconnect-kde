/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LANDEVICELINK_H
#define LANDEVICELINK_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QSslSocket>
#include <QSslCertificate>

#include <kdeconnectcore_export.h>
#include "backends/devicelink.h"
#include "uploadjob.h"
#include "compositeuploadjob.h"

class SocketLineReader;

class KDECONNECTCORE_EXPORT LanDeviceLink
    : public DeviceLink
{
    Q_OBJECT

public:
    enum ConnectionStarted : bool { Locally, Remotely };

    LanDeviceLink(const QString& deviceId, LinkProvider* parent, QSslSocket* socket, ConnectionStarted connectionSource);
    void reset(QSslSocket* socket, ConnectionStarted connectionSource);

    QString name() override;
    bool sendPacket(NetworkPacket& np) override;

    void userRequestsPair() override;
    void userRequestsUnpair() override;

    void setPairStatus(PairStatus status) override;

    bool linkShouldBeKeptAlive() override;

    QHostAddress hostAddress() const;
    QSslCertificate certificate() const;

private Q_SLOTS:
    void dataReceived();

private:
    SocketLineReader* m_socketLineReader;
    ConnectionStarted m_connectionSource;
    QPointer<CompositeUploadJob> m_compositeUploadJob;
};

#endif
