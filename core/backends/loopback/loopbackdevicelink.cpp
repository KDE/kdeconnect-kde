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

#include "loopbackdevicelink.h"

#include "loopbacklinkprovider.h"

LoopbackDeviceLink::LoopbackDeviceLink(const QString& deviceId, LoopbackLinkProvider* provider)
    : DeviceLink(deviceId, provider)
{

}

QString LoopbackDeviceLink::name()
{
    return QStringLiteral("LoopbackLink");
}

bool LoopbackDeviceLink::sendPackage(NetworkPackage& input)
{
    NetworkPackage output(QString::null);
    NetworkPackage::unserialize(input.serialize(), &output);

    //LoopbackDeviceLink does not need deviceTransferInfo
    if (input.hasPayload()) {
        bool b = input.payload()->open(QIODevice::ReadOnly);
        Q_ASSERT(b);
        output.setPayload(input.payload(), input.payloadSize());
    }

    Q_EMIT receivedPackage(output);

    return true;
}

