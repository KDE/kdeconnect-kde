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

#include "core/backends/lan/mdns.h"
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

        MdnshWrapper::Announcer announcer(instanceName, serviceType, 0);
        announcer.setPort(instancePort);
        announcer.putTxtRecord(txtKey, txtValue);

        MdnshWrapper::Announcer unrelatedAnnouncer(QStringLiteral("unrelated"), QStringLiteral("_unrelated._udp.local"), 1234);

        MdnshWrapper::Discoverer discoverer;
        // Multicast responses are received by the announcers, which listen on the mDNS port
        connect(&announcer, &MdnshWrapper::Announcer::responseReceived, &discoverer, &MdnshWrapper::Discoverer::handleResponse);
        connect(&unrelatedAnnouncer, &MdnshWrapper::Announcer::responseReceived, &discoverer, &MdnshWrapper::Discoverer::handleResponse);

        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);

        connect(&discoverer,
                &MdnshWrapper::Discoverer::serviceFound,
                this,
                [instanceName, instancePort, serviceType, txtKey, txtValue](const MdnshWrapper::Discoverer::MdnsService &service) {
                    QCOMPARE(instanceName, service.name);
                    QCOMPARE(serviceType + QLatin1Char('.'), service.serviceType);
                    QCOMPARE(instancePort, service.port);
                    QVERIFY(service.txtRecords.size() == 1);
                    QVERIFY(service.txtRecords.contains(txtKey));
                    QCOMPARE(txtValue, service.txtRecords.value(txtKey));
                });

        discoverer.startDiscovering(serviceType);
        unrelatedAnnouncer.startAnnouncing();
        QTest::qWait(100);
        QCOMPARE(spy.count(), 0);

        // The announcement is received several times (IPv4 and IPv6, by both announcers) but reported once
        announcer.startAnnouncing();
        QVERIFY(spy.wait(2000));
        QTest::qWait(100);
        QCOMPARE(spy.count(), 1);

        // Repeated responses for a known service are not reported again
        announcer.sendMulticastAnnounce(false);
        QTest::qWait(100);
        QCOMPARE(spy.count(), 1);

        // Goodbye records are not reported as discoveries (unrelatedAnnouncer is still listening)
        announcer.stopAnnouncing();
        QTest::qWait(100);
        QCOMPARE(spy.count(), 1);

        // After a goodbye, the service is reported again when it comes back
        announcer.startAnnouncing();
        QVERIFY(spy.wait(2000));
        QTest::qWait(100);
        QCOMPARE(spy.count(), 2);

        announcer.stopAnnouncing();
        unrelatedAnnouncer.stopAnnouncing();
        discoverer.stopDiscovering();
    }

    void testAnswerToQuery()
    {
        QString serviceType = QStringLiteral("_test._udp.local");
        QByteArray serviceTypeBytes = serviceType.toLatin1() + '.';

        MdnshWrapper::Announcer announcer(QStringLiteral("myinstance"), serviceType, 1716);
        MdnshWrapper::Announcer otherAnnouncer(QStringLiteral("other"), QStringLiteral("_other._udp.local"), 1234);

        MdnshWrapper::Discoverer discoverer;
        connect(&otherAnnouncer, &MdnshWrapper::Announcer::responseReceived, &discoverer, &MdnshWrapper::Discoverer::handleResponse);

        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);

        announcer.startAnnouncing();
        otherAnnouncer.startAnnouncing();

        // Sent from the mDNS port, so it asks for a multicast answer (on some systems, like macOS, the
        // queries sent by the Discoverer from other ports don't reach announcers on the same host)
        sockaddr_in sockAddr;
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(MDNS_PORT);
#ifdef __APPLE__
        sockAddr.sin_len = sizeof(sockAddr);
#endif
        int querySocket = mdns_socket_open_ipv4(&sockAddr);
        QVERIFY(querySocket >= 0);
        char buffer[2048];
        auto sendQuery = [&]() {
            QVERIFY(mdns_query_send(querySocket, MDNS_RECORDTYPE_PTR, serviceTypeBytes.constData(), serviceTypeBytes.size(), buffer, sizeof(buffer), 0) >= 0);
        };

        // The answers to the Discoverer's own query may or may not arrive, depending on the system
        discoverer.startDiscovering(serviceType);
        QTest::qWait(200);
        int count = spy.count();
        QVERIFY(count <= 1);

        // A new query reports the service once, even if it's in the cache and answers arrive on several sockets
        discoverer.sendQuery(serviceType);
        sendQuery();
        QVERIFY(spy.wait(2000));
        QTest::qWait(200);
        QCOMPARE(spy.count(), count + 1);

        // Answers and announcements in the same burst are not reported again
        sendQuery();
        announcer.sendMulticastAnnounce(false);
        QTest::qWait(200);
        QCOMPARE(spy.count(), count + 1);

        mdns_socket_close(querySocket);
        discoverer.stopDiscovering();
        otherAnnouncer.stopAnnouncing();
        announcer.stopAnnouncing();
    }

    void testCacheExpiry_data()
    {
        QTest::addColumn<uint32_t>("ttl");
        QTest::newRow("short-ttl") << uint32_t(1);
        QTest::newRow("long-ttl") << uint32_t(120);
    }

    void testCacheExpiry()
    {
        QFETCH(uint32_t, ttl);
        const QString serviceType = QStringLiteral("_test._udp.local");
        MdnshWrapper::Discoverer discoverer;
        discoverer.startDiscovering(serviceType);
        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);

        MdnshWrapper::Discoverer::MdnsService service;
        service.name = QStringLiteral("retry");
        service.serviceType = serviceType + QLatin1Char('.');
        service.port = 1716;
        service.address = QHostAddress(QStringLiteral("192.0.2.1"));
        service.ttl = ttl;

        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 1);
        QTest::qWait(600);
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 1);

        // Duplicates must not extend suppression indefinitely.
        QTest::qWait(600);
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 2);
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 2);

        discoverer.stopDiscovering();
    }

    void testCacheTxtChanges()
    {
        const QString serviceType = QStringLiteral("_test._udp.local");
        MdnshWrapper::Discoverer discoverer;
        discoverer.startDiscovering(serviceType);
        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);

        MdnshWrapper::Discoverer::MdnsService service;
        service.name = QStringLiteral("dev");
        service.serviceType = serviceType + QLatin1Char('.');
        service.port = 1716;
        service.address = QHostAddress(QStringLiteral("192.0.2.1"));
        service.ttl = 120;

        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 1);

        // A complete response must be reported after one missing TXT metadata.
        service.txtRecords.insert(QStringLiteral("protocol"), QStringLiteral("7"));
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(qvariant_cast<MdnshWrapper::Discoverer::MdnsService>(spy.last().first()).txtRecords, service.txtRecords);

        // Corrected metadata must also be reported when the port hasn't changed.
        service.txtRecords.insert(QStringLiteral("protocol"), QStringLiteral("8"));
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 3);
        QCOMPARE(qvariant_cast<MdnshWrapper::Discoverer::MdnsService>(spy.last().first()).txtRecords, service.txtRecords);

        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 3);

        // A response without TXT records after a complete one is not a change
        service.txtRecords.clear();
        discoverer.handleResponse(service);
        QCOMPARE(spy.count(), 3);

        discoverer.stopDiscovering();
    }

    void testCache()
    {
        QString serviceType = QStringLiteral("_test._udp.local");

        MdnshWrapper::Discoverer discoverer;
        QSignalSpy spy(&discoverer, &MdnshWrapper::Discoverer::serviceFound);
        discoverer.startDiscovering(serviceType);
        QTest::qWait(100);
        spy.clear();

        auto service = [&](const QString &name, uint16_t port, const QString &address, uint32_t ttl) {
            MdnshWrapper::Discoverer::MdnsService service;
            service.name = name;
            service.serviceType = serviceType + QLatin1Char('.');
            service.port = port;
            service.address = QHostAddress(address);
            service.ttl = ttl;
            return service;
        };

        discoverer.handleResponse(service(QStringLiteral("dev"), 1716, QStringLiteral("192.168.1.10"), 10));
        QCOMPARE(spy.count(), 1);

        // Same service with another address, case or TTL: not reported again
        discoverer.handleResponse(service(QStringLiteral("DEV"), 1716, QStringLiteral("fe80::1"), 120));
        QCOMPARE(spy.count(), 1);

        // Port changed (eg: the device restarted without a goodbye): reported
        discoverer.handleResponse(service(QStringLiteral("dev"), 1717, QStringLiteral("192.168.1.10"), 120));
        QCOMPARE(spy.count(), 2);

        // Incomplete response without SRV record: ignored, and doesn't replace the cached port
        discoverer.handleResponse(service(QStringLiteral("dev"), 0, QStringLiteral("192.168.1.10"), 120));
        discoverer.handleResponse(service(QStringLiteral("dev"), 1717, QStringLiteral("192.168.1.10"), 120));
        QCOMPARE(spy.count(), 2);

        // Other service types are ignored
        MdnshWrapper::Discoverer::MdnsService other = service(QStringLiteral("dev2"), 1716, QStringLiteral("192.168.1.11"), 120);
        other.serviceType = QStringLiteral("_other._udp.local.");
        discoverer.handleResponse(other);
        QCOMPARE(spy.count(), 2);

        // Goodbye: not reported, and the service is reported again when it comes back
        discoverer.handleResponse(service(QStringLiteral("dev"), 1717, QStringLiteral("192.168.1.10"), 0));
        QCOMPARE(spy.count(), 2);
        discoverer.handleResponse(service(QStringLiteral("dev"), 1717, QStringLiteral("192.168.1.10"), 120));
        QCOMPARE(spy.count(), 3);

        // A new query reports known services again
        discoverer.sendQuery(serviceType);
        discoverer.handleResponse(service(QStringLiteral("dev"), 1717, QStringLiteral("192.168.1.10"), 120));
        QCOMPARE(spy.count(), 4);

        // Not reported when not discovering
        discoverer.stopDiscovering();
        discoverer.handleResponse(service(QStringLiteral("dev3"), 1716, QStringLiteral("192.168.1.12"), 120));
        QCOMPARE(spy.count(), 4);
    }
};

QTEST_MAIN(TestMdns);

#include "mdnstest.moc"
