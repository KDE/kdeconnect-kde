/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "loopbacklinkprovider.h"

#include "core_debug.h"

LoopbackLinkProvider::LoopbackLinkProvider(bool disabled)
    : enabled(!disabled)
{
}

LoopbackLinkProvider::~LoopbackLinkProvider()
{
}

void LoopbackLinkProvider::onNetworkChange()
{
    if (!enabled) {
        if (loopbackDeviceLink) {
            delete loopbackDeviceLink;
        }
        return;
    }

    LoopbackDeviceLink *newLoopbackDeviceLink = new LoopbackDeviceLink(this);
    Q_EMIT onConnectionReceived(newLoopbackDeviceLink);

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

#include "moc_loopbacklinkprovider.cpp"
