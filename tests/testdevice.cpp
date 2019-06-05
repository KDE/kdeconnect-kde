/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
 * Copyright 2019 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "testdevice.h"

TestDevice::TestDevice(QObject* parent, const QString& id)
    : Device (parent, id)
    , sentPackets(0)
    , lastPacket(nullptr)
{}

TestDevice::~TestDevice()
{
    delete lastPacket;
}

bool TestDevice::isReachable() const
{
    return true;
}

bool TestDevice::sendPacket(NetworkPacket& np)
{
    ++sentPackets;
    // copy packet manually to allow for inspection (can't use copy-constructor)
    deleteLastPacket();
    lastPacket = new NetworkPacket(np.type());
    Q_ASSERT(lastPacket);
    for (QVariantMap::ConstIterator iter = np.body().constBegin();
         iter != np.body().constEnd(); iter++)
        lastPacket->set(iter.key(), iter.value());
    lastPacket->setPayload(np.payload(), np.payloadSize());
    return true;
}
