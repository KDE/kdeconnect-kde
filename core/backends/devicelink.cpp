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
    connect(this, &QObject::destroyed, [this, deviceId, parent]() {
        parent->onLinkDestroyed(deviceId, this);
    });
    this->priorityFromProvider = parent->priority();
    this->displayNameFromProvider = parent->displayName();
    this->nameFromProvider = parent->name();
}

#include "moc_devicelink.cpp"
