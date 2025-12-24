/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTest>

#include "core/backends/lan/mdnsh_wrapper.h"
#include "kdeconnect-version.h"

class TestMdns : public QObject
{
    Q_OBJECT
public:
    TestMdns()
    {
    }

private Q_SLOTS:
    void testAnounceAndDiscover()
    {
        QString instanceName = QStringLiteral("myinstance");
        uint16_t instancePort = 1716;
        QString serviceType = QStringLiteral("_test._udp.local");
        QString txtKey = QStringLiteral("keyerino");
        QString txtValue = QStringLiteral("valuerino");

        MdnshWrapper::Announcer announcer(instanceName, serviceType, instancePort);
        announcer.putTxtRecord(txtKey, txtValue);

        MdnshWrapper::Discoverer discoverer;

        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);

        connect(&discoverer,
                &MdnshWrapper::Discoverer::serviceFound,
                this,
                [instanceName, instancePort, txtKey, txtValue](const MdnshWrapper::Discoverer::MdnsService &service) {
                    QCOMPARE(instanceName, service.name);
                    QCOMPARE(instancePort, service.port);
                    QVERIFY(service.txtRecords.size() == 1);
                    QVERIFY(service.txtRecords.contains(txtKey));
                    QCOMPARE(txtValue, service.txtRecords.value(txtKey));
                });

        announcer.startAnnouncing();
        discoverer.startDiscovering(serviceType);

        QVERIFY(spy.wait(2000));
        QVERIFY(spy.count() > 0);

        discoverer.stopDiscovering();
        announcer.stopAnnouncing();
    }
};

QTEST_MAIN(TestMdns);

#include "mdnstest.moc"
