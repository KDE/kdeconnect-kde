/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "loopbackdevicelink.h"

#include "loopbacklinkprovider.h"

LoopbackDeviceLink::LoopbackDeviceLink(const QString &deviceId, LoopbackLinkProvider *provider)
    : DeviceLink(deviceId, provider)
{
}

QString LoopbackDeviceLink::name()
{
    return QStringLiteral("LoopbackLink");
}

bool LoopbackDeviceLink::sendPacket(NetworkPacket &input)
{
    NetworkPacket output((QString()));
    NetworkPacket::unserialize(input.serialize(), &output);

    // LoopbackDeviceLink does not need deviceTransferInfo
    if (input.hasPayload()) {
        bool b = input.payload()->open(QIODevice::ReadOnly);
        Q_ASSERT(b);
        output.setPayload(input.payload(), input.payloadSize());
    }

    Q_EMIT receivedPacket(output);

    return true;
}
