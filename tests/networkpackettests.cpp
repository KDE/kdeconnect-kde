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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "networkpackettests.h"

#include "core/networkpacket.h"

#include <QtTest>

QTEST_GUILESS_MAIN(NetworkPacketTests);

void NetworkPacketTests::initTestCase()
{
    // Called before the first testfunction is executed
}

void NetworkPacketTests::networkPacketTest()
{
    NetworkPacket np(QStringLiteral("com.test"));

    np.set(QStringLiteral("hello"), QStringLiteral("hola"));
    QCOMPARE( (np.get<QString>(QStringLiteral("hello"), QStringLiteral("bye"))) , QStringLiteral("hola") );

    np.set(QStringLiteral("hello"), QString());
    QCOMPARE((np.get<QString>(QStringLiteral("hello"), QStringLiteral("bye"))) , QString());

    np.body().remove(QStringLiteral("hello"));
    QCOMPARE((np.get<QString>(QStringLiteral("hello"), QStringLiteral("bye"))) , QStringLiteral("bye"));

    np.set(QStringLiteral("foo"), QStringLiteral("bar"));
    QByteArray ba = np.serialize();
    //qDebug() << "Serialized packet:" << ba;
    NetworkPacket np2(QLatin1String(""));
    NetworkPacket::unserialize(ba,&np2);

    QCOMPARE( np.id(), np2.id() );
    QCOMPARE( np.type(), np2.type() );
    QCOMPARE( np.body(), np2.body() );

    QByteArray json("{\"id\":\"123\",\"type\":\"test\",\"body\":{\"testing\":true}}");
    //qDebug() << json;
    NetworkPacket::unserialize(json,&np2);
    QCOMPARE( np2.id(), QStringLiteral("123") );
    QCOMPARE( (np2.get<bool>(QStringLiteral("testing"))), true );
    QCOMPARE( (np2.get<bool>(QStringLiteral("not_testing"))), false );
    QCOMPARE( (np2.get<bool>(QStringLiteral("not_testing"),true)), true );

    //NetworkPacket::unserialize("this is not json",&np2);
    //QtTest::ignoreMessage(QtSystemMsg, "json_parser - syntax error found,  forcing abort, Line 1 Column 0");
    //QtTest::ignoreMessage(QtDebugMsg, "Unserialization error: 1 \"syntax error, unexpected string\"");

}

void NetworkPacketTests::networkPacketIdentityTest()
{
    NetworkPacket np(QLatin1String(""));
    NetworkPacket::createIdentityPacket(&np);

    QCOMPARE( np.get<int>(QStringLiteral("protocolVersion"), -1) , NetworkPacket::s_protocolVersion );
    QCOMPARE( np.type() , PACKET_TYPE_IDENTITY );

}

void NetworkPacketTests::cleanupTestCase()
{
    // Called after the last testfunction was executed
}

void NetworkPacketTests::init()
{
    // Called before each testfunction is executed
}

void NetworkPacketTests::cleanup()
{
    // Called after every testfunction
}

