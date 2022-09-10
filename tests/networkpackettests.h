/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NETWORKPACKETTESTS_H
#define NETWORKPACKETTESTS_H

#include <QObject>

class NetworkPacketTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void networkPacketTest();
    void networkPacketIdentityTest();
    void networkPacketPayloadTest();
    // void networkPacketEncryptionTest();

    void cleanupTestCase();

    void init();
    void cleanup();
};

#endif
