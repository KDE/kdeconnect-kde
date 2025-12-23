/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicelink.h"

#include "linkprovider.h"

DeviceLink::DeviceLink(LinkProvider *linkProvider)
    : QObject()
{
    this->priorityFromProvider = linkProvider->priority();
    this->displayNameFromProvider = linkProvider->displayName();
    this->nameFromProvider = linkProvider->name();
}

#include "moc_devicelink.cpp"
