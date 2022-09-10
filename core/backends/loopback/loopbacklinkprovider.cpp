/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "loopbacklinkprovider.h"

#include "core_debug.h"

LoopbackLinkProvider::LoopbackLinkProvider()
    : identityPacket(PACKET_TYPE_IDENTITY)
{
    NetworkPacket::createIdentityPacket(&identityPacket);
}

LoopbackLinkProvider::~LoopbackLinkProvider()
{
}

void LoopbackLinkProvider::onNetworkChange()
{
    LoopbackDeviceLink *newLoopbackDeviceLink = new LoopbackDeviceLink(QStringLiteral("loopback"), this);
    Q_EMIT onConnectionReceived(identityPacket, newLoopbackDeviceLink);

    if (loopbackDeviceLink) {
        delete loopbackDeviceLink;
    }

    loopbackDeviceLink = newLoopbackDeviceLink;
}

void LoopbackLinkProvider::onStart()
{
    onNetworkChange();
}

void LoopbackLinkProvider::onStop()
{
    if (loopbackDeviceLink) {
        delete loopbackDeviceLink;
    }
}
