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

#ifndef TESTDEVICE_H
#define TESTDEVICE_H

#include <QtCore>
#include "core/device.h"

// Tweaked Device for testing:
class TestDevice: public Device
{
    Q_OBJECT
private:
    int sentPackets;
    NetworkPacket* lastPacket;

public:
    explicit TestDevice(QObject* parent, const QString& id);

    ~TestDevice() override;

    bool isReachable() const override;

    int getSentPackets() const
    {
        return sentPackets;
    }

    NetworkPacket* getLastPacket()
    {
        return lastPacket;
    }

private:
    void deleteLastPacket()
    {
        delete lastPacket;
        lastPacket = nullptr;
    }

public Q_SLOTS:
    bool sendPacket(NetworkPacket& np) override;

};

#endif
