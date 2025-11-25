/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LOOPBACKDEVICELINK_H
#define LOOPBACKDEVICELINK_H

#include "../devicelink.h"
#include "deviceinfo.h"

class LoopbackLinkProvider;

class LoopbackDeviceLink : public DeviceLink
{
    Q_OBJECT
public:
    LoopbackDeviceLink(LoopbackLinkProvider *a);

    virtual bool sendPacket(NetworkPacket &np) override;

    virtual DeviceInfo deviceInfo() const override;

    QString address() const override
    {
        return QStringLiteral("127.0.0.1");
    }
};

#endif
