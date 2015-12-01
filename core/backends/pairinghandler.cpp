/**
 * Copyright 2015 Vineet Garg <grg.vineet@gmail.com>
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

#include "pairinghandler.h"

PairingHandler::PairingHandler()
    : m_deviceLink(nullptr)
    , m_pairStatus(NotPaired)
{

}

void PairingHandler::setDeviceLink(DeviceLink *dl)
{
    m_deviceLink =  dl;
}

DeviceLink* PairingHandler::deviceLink() const
{
    return m_deviceLink;
}

void PairingHandler::linkDestroyed(QObject* o)
{
    DeviceLink* dl = static_cast<DeviceLink*>(o);
    if (dl == m_deviceLink) { // Check if same link is destroyed
        m_deviceLink = Q_NULLPTR;
        Q_EMIT linkNull();
    }
}

void PairingHandler::setPairStatus(PairingHandler::PairStatus status)
{
    if (m_pairStatus != status) {
        PairStatus oldStatus = m_pairStatus;
        m_pairStatus = status;
        Q_EMIT pairStatusChanged(status, oldStatus);
    }
}

PairingHandler::PairStatus PairingHandler::pairStatus() const
{
    return m_pairStatus;
}

