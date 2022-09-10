/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pairinghandler.h"

PairingHandler::PairingHandler(DeviceLink *parent)
    : QObject(parent)
    , m_deviceLink(parent)
{
}

void PairingHandler::setDeviceLink(DeviceLink *dl)
{
    setParent(dl);
    m_deviceLink = dl;
}

DeviceLink *PairingHandler::deviceLink() const
{
    return m_deviceLink;
}
