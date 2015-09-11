/**
 * Copyright 2015 Vineet Garg <grgvineet@gmail.com>
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

#include "../core/backends/lan/lanlinkprovider.h"
#include "../core/kdeconnectconfig.h"

#include <QtTest>

/**
 * This class tests the working of device class
 */
class DeviceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase();
    void testUnpairedDevice();
    void testPairedDevice();
    void cleanupTestCase();

private:
    QString deviceId;
    QString deviceName;
    QString deviceType;
    NetworkPackage* identityPackage;

};

void DeviceTest::initTestCase()
{
    deviceId = QString("testdevice");
    deviceName = QString("Test Device");
    deviceType = QString("smartphone");
    QString stringPackage = QString("{\"id\":1439365924847,\"type\":\"kdeconnect.identity\",\"body\":{\"deviceId\":\"testdevice\",\"deviceName\":\"Test Device\",\"protocolVersion\":6,\"deviceType\":\"phone\"}}");
    identityPackage = new NetworkPackage("kdeconnect.identity");
    NetworkPackage::unserialize(stringPackage.toLatin1(), identityPackage);
}

void DeviceTest::testPairedDevice()
{
    KdeConnectConfig* kcc = KdeConnectConfig::instance();
    kcc->addTrustedDevice(deviceId, deviceName, deviceType);
    kcc->setDeviceProperty(deviceId, QString("publicKey"), kcc->publicKey().toPEM()); // Using same public key from kcc, instead of creating new one
    kcc->setDeviceProperty(deviceId, QString("certificate"), QString::fromLatin1(kcc->certificate().toPem())); // Using same certificate from kcc, instead of generating one

    Device device(this, deviceId);

    QCOMPARE(device.id(), deviceId);
    QCOMPARE(device.name(), deviceName);
    QCOMPARE(device.type(), deviceType);

    QVERIFY2(!device.publicKey().isNull(), "Public key for device is null");
    QCOMPARE(device.publicKey(), kcc->publicKey());

    QVERIFY2(device.certificate().isNull(), "Certificate should be null before initialisation");

    // TODO : Set certificate via identity package

    QCOMPARE(device.isPaired(), true);
    QCOMPARE(device.pairRequested(), false);

    QCOMPARE(device.isReachable(), false);

    // Add link
    LanLinkProvider linkProvider;
    QSslSocket socket;
    LanDeviceLink* link = new LanDeviceLink(deviceId, &linkProvider, &socket, DeviceLink::Locally);
    device.addLink(*identityPackage, link);

    QCOMPARE(device.isReachable(), true);
    QCOMPARE(device.availableLinks().contains(linkProvider.name()), true);

    // Remove link
    device.removeLink(link);

    QCOMPARE(device.isReachable(), false);
    QCOMPARE(device.availableLinks().contains(linkProvider.name()), false);

    device.unpair();
    QCOMPARE(device.isPaired(), false);

}

void DeviceTest::testUnpairedDevice()
{
    LanLinkProvider linkProvider;
    QSslSocket socket;
    LanDeviceLink* link = new LanDeviceLink(deviceId, &linkProvider, &socket, DeviceLink::Locally);

    Device device(this, *identityPackage, link);

    QCOMPARE(device.id(), deviceId);
    QCOMPARE(device.name(), deviceName);
    QCOMPARE(device.type(), deviceType);

    QVERIFY2(device.publicKey().isNull(), "Public key for unpaired device should be null");
    QVERIFY2(device.certificate().isNull(), "Certificate for unpaired device should be null");

    QCOMPARE(device.isPaired(), false);
    QCOMPARE(device.pairRequested(), false);

    QCOMPARE(device.isReachable(), true);
    QCOMPARE(device.availableLinks().contains(linkProvider.name()), true);

    // Remove link
    device.removeLink(link);

    QCOMPARE(device.isReachable(), false);
    QCOMPARE(device.availableLinks().contains(linkProvider.name()), false);
}

void DeviceTest::cleanupTestCase()
{
    delete identityPackage;
}
QTEST_GUILESS_MAIN(DeviceTest)

#include "devicetest.moc"
