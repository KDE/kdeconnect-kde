/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2019 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "testdevice.h"

TestDevice::TestDevice(QObject *parent, const QString &id)
    : Device(parent, id)
    , sentPackets(0)
    , lastPacket(nullptr)
{
}

TestDevice::~TestDevice()
{
    delete lastPacket;
}

bool TestDevice::isReachable() const
{
    return true;
}

bool TestDevice::sendPacket(NetworkPacket &np)
{
    ++sentPackets;
    // copy packet manually to allow for inspection (can't use copy-constructor)
    deleteLastPacket();
    lastPacket = new NetworkPacket(np.type());
    Q_ASSERT(lastPacket);
    for (QVariantMap::ConstIterator iter = np.body().constBegin(); iter != np.body().constEnd(); iter++)
        lastPacket->set(iter.key(), iter.value());
    lastPacket->setPayload(np.payload(), np.payloadSize());
    return true;
}

#include "moc_testdevice.cpp"
