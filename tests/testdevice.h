/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2019 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef TESTDEVICE_H
#define TESTDEVICE_H

#include "core/device.h"
#include <QtCore>

// Tweaked Device for testing:
class TestDevice : public Device
{
    Q_OBJECT
private:
    int sentPackets;
    NetworkPacket *lastPacket;

public:
    explicit TestDevice(QObject *parent, const QString &id);

    ~TestDevice() override;

    bool isReachable() const override;

    int getSentPackets() const
    {
        return sentPackets;
    }

    NetworkPacket *getLastPacket()
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
    bool sendPacket(NetworkPacket &np) override;
};

#endif
