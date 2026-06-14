/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicelink.h"

#include "linkprovider.h"

DeviceLink::DeviceLink(const QString &deviceId, LinkProvider *parent)
    : QObject(parent)
{
    connect(this, &QObject::destroyed, parent, [this, deviceId, parent]() {
        parent->onLinkDestroyed(deviceId, this);
    });
    this->priorityFromProvider = parent->priority();
    this->displayidFromProvider = parent->displayName();
    this->idFromProvider = parent->id();
}

#include "moc_devicelink.cpp"
