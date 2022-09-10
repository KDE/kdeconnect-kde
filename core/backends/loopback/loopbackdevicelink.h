/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LOOPBACKDEVICELINK_H
#define LOOPBACKDEVICELINK_H

#include "../devicelink.h"
#include <QSslCertificate>

class LoopbackLinkProvider;

class LoopbackDeviceLink : public DeviceLink
{
    Q_OBJECT
public:
    LoopbackDeviceLink(const QString &d, LoopbackLinkProvider *a);

    QString name() override;
    bool sendPacket(NetworkPacket &np) override;

    void userRequestsPair() override
    {
        setPairStatus(Paired);
    }
    void userRequestsUnpair() override
    {
        setPairStatus(NotPaired);
    }

    QSslCertificate certificate() const override
    {
        return QSslCertificate();
    }
};

#endif
