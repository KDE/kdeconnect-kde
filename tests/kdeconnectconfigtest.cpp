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



#include "../core/kdeconnectconfig.h"

#include <QtTest>

/*
 * This class tests the working of kdeconnect config that certificate and key is generated and saved properly
 */
class KdeConnectConfigTest : public QObject
{
Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void addTrustedDevice();
/*
    void remoteCertificateTest();
*/
    void removeTrustedDevice();

private:
    KdeConnectConfig* kcc;
};

void KdeConnectConfigTest::initTestCase()
{
    kcc = KdeConnectConfig::instance();
}

void KdeConnectConfigTest::addTrustedDevice()
{
    kcc->addTrustedDevice(QStringLiteral("testdevice"), QStringLiteral("Test Device"), QStringLiteral("phone"));
    KdeConnectConfig::DeviceInfo devInfo = kcc->getTrustedDevice(QStringLiteral("testdevice"));
    QCOMPARE(devInfo.deviceName, QString("Test Device"));
    QCOMPARE(devInfo.deviceType, QString("phone"));
}

/*
// This checks whether certificate is generated correctly and stored correctly or not
void KdeConnectConfigTest::remoteCertificateTest()
{
    QSslCertificate certificate = kcc->certificate(); // Using same certificate as of device

    QCOMPARE(certificate.serialNumber().toInt(0,16), 10);
    QCOMPARE(certificate.subjectInfo(QSslCertificate::SubjectInfo::CommonName).first(), kcc->deviceId());
    QCOMPARE(certificate.subjectInfo(QSslCertificate::SubjectInfo::Organization).first(), QString("KDE"));
    QCOMPARE(certificate.subjectInfo(QSslCertificate::OrganizationalUnitName).first(), QString("Kde connect"));

    kcc->setDeviceProperty("testdevice","certificate", QString::fromLatin1(certificate.toPem()));

    KdeConnectConfig::DeviceInfo devInfo = kcc->getTrustedDevice("testdevice");
    QSslCertificate devCertificate = QSslCertificate::fromData(devInfo.certificate.toLatin1()).first();

    QCOMPARE(devCertificate.serialNumber().toInt(0,16), 10);
    QCOMPARE(devCertificate.subjectInfo(QSslCertificate::SubjectInfo::CommonName).first(), kcc->deviceId());
    QCOMPARE(devCertificate.subjectInfo(QSslCertificate::SubjectInfo::Organization).first(), QString("KDE"));
    QCOMPARE(devCertificate.subjectInfo(QSslCertificate::OrganizationalUnitName).first(), QString("Kde connect"));

}
*/


void KdeConnectConfigTest::removeTrustedDevice()
{
    kcc->removeTrustedDevice(QStringLiteral("testdevice"));
    KdeConnectConfig::DeviceInfo devInfo = kcc->getTrustedDevice(QStringLiteral("testdevice"));
    QCOMPARE(devInfo.deviceName, QString("unnamed"));
    QCOMPARE(devInfo.deviceType, QString("unknown"));
}

QTEST_GUILESS_MAIN(KdeConnectConfigTest)

#include "kdeconnectconfigtest.moc"
