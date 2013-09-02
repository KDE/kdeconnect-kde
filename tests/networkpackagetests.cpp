/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "networkpackagetests.h"

#include "../daemon/networkpackage.h"

#include <qtest_kde.h>
#include <QtTest>

QTEST_KDEMAIN(NetworkPackageTests, NoGUI);

void NetworkPackageTests::initTestCase()
{
    // Called before the first testfunction is executed
}

void NetworkPackageTests::dummyTest()
{
    QDate date;
    date.setYMD( 1967, 3, 11 );
    QVERIFY( date.isValid() );
    QCOMPARE( date.month(), 3 );
    QCOMPARE( QDate::longMonthName(date.month()), QString("March") );

    QCOMPARE(QString("hello").toUpper(), QString("HELLO"));
}

void NetworkPackageTests::networkPackageTest()
{
    NetworkPackage np("com.test");

    np.set("hello","hola");
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("hola") );

    np.set("hello","");
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("") );

    np.body().remove("hello");
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("bye") );

    QByteArray ba = np.serialize();
    //qDebug() << "Serialized package:" << ba;
    NetworkPackage np2("");
    NetworkPackage::unserialize(ba,&np2);

    QCOMPARE( np.id(), np2.id() );
    QCOMPARE( np.type(), np2.type() );
    QCOMPARE( np.version(), np2.version() );
    QCOMPARE( np.body(), np2.body() );

    QByteArray json("{ \"id\": \"123\", \"type\": \"test\", \"body\": { \"testing\": true }, \"version\": 3 }");
    //qDebug() << json;
    NetworkPackage::unserialize(json,&np2);
    QCOMPARE( np2.id(), QString("123") );
    QCOMPARE( np2.version(), 3 );
    QCOMPARE( (np2.get<bool>("testing")), true );
    QCOMPARE( (np2.get<bool>("not_testing")), false );
    QCOMPARE( (np2.get<bool>("not_testing",true)), true );

    //NetworkPackage::unserialize("this is not json",&np2);
    //QtTest::ignoreMessage(QtSystemMsg, "json_parser - syntax error found,  forcing abort, Line 1 Column 0");
    //QtTest::ignoreMessage(QtDebugMsg, "Unserialization error: 1 \"syntax error, unexpected string\"");
    //QCOMPARE( np2.version(), -1 );


}

void NetworkPackageTests::networkPackageEncryptionTest()
{

    NetworkPackage original("com.test");
    original.set("hello","hola");

    NetworkPackage copy("");
    NetworkPackage::unserialize(original.serialize(), &copy);

    NetworkPackage decrypted("");

    QCA::Initializer init;
    QCA::PrivateKey privateKey = QCA::KeyGenerator().createRSA(2048);
    QCA::PublicKey publicKey = privateKey.toPublicKey();


    //Encrypt and decrypt np
    QCOMPARE( original.isEncrypted(), false );
    original.encrypt(publicKey);
    QCOMPARE( original.isEncrypted(), true );
    original.decrypt(privateKey, &decrypted);
    QCOMPARE( original.isEncrypted(), true );
    QCOMPARE( decrypted.isEncrypted(), false );

    //np should be equal top np2
    QCOMPARE( decrypted.id(), copy.id() );
    QCOMPARE( decrypted.type(), copy.type() );
    QCOMPARE( decrypted.version(), copy.version() );
    QCOMPARE( decrypted.body(), copy.body() );



    //Test for long package encryption that need multi-chunk encryption

    QByteArray json = "{ \"body\" : { \"nowPlaying\" : \"A really long song name - A really long artist name\", \"player\" : \"A really long player name\" }, \"id\" : \"A really long package id\", \"isEncrypted\" : false, \"type\" : \"kdeconnect.a_really_long_package_type\", \"version\" : 2 }\n";
    qDebug() << "EME_PKCS1_OAEP maximumEncryptSize" << publicKey.maximumEncryptSize(QCA::EME_PKCS1_OAEP);
    qDebug() << "EME_PKCS1v15 maximumEncryptSize" << publicKey.maximumEncryptSize(QCA::EME_PKCS1v15);
    QCOMPARE( json.size() > publicKey.maximumEncryptSize(QCA::EME_PKCS1v15), true );

    NetworkPackage::unserialize(json, &original);
    original.encrypt(publicKey);
    original.decrypt(privateKey, &decrypted);
    QByteArray decryptedJson = decrypted.serialize();

    QCOMPARE(QString(json), QString(decryptedJson));

}


void NetworkPackageTests::cleanupTestCase()
{
    // Called after the last testfunction was executed
}

void NetworkPackageTests::init()
{
    // Called before each testfunction is executed
}

void NetworkPackageTests::cleanup()
{
    // Called after every testfunction
}

