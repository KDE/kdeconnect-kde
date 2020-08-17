/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicelink.h"
#include "kdeconnectconfig.h"
#include "linkprovider.h"

DeviceLink::DeviceLink(const QString& deviceId, LinkProvider* parent)
    : QObject(parent)
    , m_deviceId(deviceId)
    , m_linkProvider(parent)
    , m_pairStatus(NotPaired)
{
    Q_ASSERT(!deviceId.isEmpty());

    setProperty("deviceId", deviceId);
}

void DeviceLink::setPairStatus(DeviceLink::PairStatus status)
{
    if (m_pairStatus != status) {
        m_pairStatus = status;
        Q_EMIT pairStatusChanged(status);
    }
}

