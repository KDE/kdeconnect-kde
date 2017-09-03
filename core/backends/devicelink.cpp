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

#include "devicelink.h"
#include "kdeconnectconfig.h"
#include "linkprovider.h"

DeviceLink::DeviceLink(const QString& deviceId, LinkProvider* parent)
    : QObject(parent)
    , m_privateKey(KdeConnectConfig::instance()->privateKey())
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

