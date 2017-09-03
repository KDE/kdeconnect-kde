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

#include "core/networkpackage.h"

#include <QtTest>
#include <QtCrypto>

QTEST_GUILESS_MAIN(NetworkPackageTests);

void NetworkPackageTests::initTestCase()
{
    // Called before the first testfunction is executed
}

void NetworkPackageTests::networkPackageTest()
{
    NetworkPackage np(QStringLiteral("com.test"));

    np.set(QStringLiteral("hello"),"hola");
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("hola") );

    np.set(QStringLiteral("hello"),"");
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("") );

    np.body().remove(QStringLiteral("hello"));
    QCOMPARE( (np.get<QString>("hello","bye")) , QString("bye") );

    np.set(QStringLiteral("foo"), "bar");
    QByteArray ba = np.serialize();
    //qDebug() << "Serialized package:" << ba;
    NetworkPackage np2(QLatin1String(""));
    NetworkPackage::unserialize(ba,&np2);

    QCOMPARE( np.id(), np2.id() );
    QCOMPARE( np.type(), np2.type() );
    QCOMPARE( np.body(), np2.body() );

    QByteArray json("{\"id\":\"123\",\"type\":\"test\",\"body\":{\"testing\":true}}");
    //qDebug() << json;
    NetworkPackage::unserialize(json,&np2);
    QCOMPARE( np2.id(), QString("123") );
    QCOMPARE( (np2.get<bool>("testing")), true );
    QCOMPARE( (np2.get<bool>("not_testing")), false );
    QCOMPARE( (np2.get<bool>("not_testing",true)), true );

    //NetworkPackage::unserialize("this is not json",&np2);
    //QtTest::ignoreMessage(QtSystemMsg, "json_parser - syntax error found,  forcing abort, Line 1 Column 0");
    //QtTest::ignoreMessage(QtDebugMsg, "Unserialization error: 1 \"syntax error, unexpected string\"");

}

void NetworkPackageTests::networkPackageIdentityTest()
{
    NetworkPackage np(QLatin1String(""));
    NetworkPackage::createIdentityPackage(&np);

    QCOMPARE( np.get<int>("protocolVersion", -1) , NetworkPackage::s_protocolVersion );
    QCOMPARE( np.type() , PACKAGE_TYPE_IDENTITY );

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

