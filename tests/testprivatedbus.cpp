/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "dbushelper.h"

#include <QtTest>
#include <QDBusMessage>
#include <QDBusConnectionInterface>

/**
 * This class tests the working of private dbus in kdeconnect
 */
class PrivateDBusTest : public QObject
{
    Q_OBJECT

public:
    PrivateDBusTest()
    {
        DBusHelper::launchDBusDaemon();
    }

    ~PrivateDBusTest()
    {
        DBusHelper::closeDBusDaemon();
    }

private Q_SLOTS:
    void testConnectionWithPrivateDBus();
    void testServiceRegistrationWithPrivateDBus();
    void testMethodCallWithPrivateDBus();
};

/**
 * Open private DBus normally and get connection info
 */
void PrivateDBusTest::testConnectionWithPrivateDBus()
{
    QDBusConnection conn = DBusHelper::sessionBus();

    QVERIFY2(conn.isConnected(), "Connection not established");
    QVERIFY2(conn.name() == QStringLiteral(KDECONNECT_PRIVATE_DBUS_NAME), 
            "DBus Connection is not the right one");
}

/**
 * Open private DBus connection normally and register a service
 */
void PrivateDBusTest::testServiceRegistrationWithPrivateDBus()
{
    QDBusConnection conn = DBusHelper::sessionBus();
    QVERIFY2(conn.isConnected(), "DBus not connected");

    QDBusConnectionInterface *bus = conn.interface();
    QVERIFY2(bus != nullptr, "Failed to get DBus interface");

    QVERIFY2(bus->registerService(QStringLiteral("privatedbus.test")) == QDBusConnectionInterface::ServiceRegistered,
        "Failed to register DBus Serice");

    bus->unregisterService(QStringLiteral("privatedbus.test"));
}

/**
 * Open private DBus connection normally, call a method and get its reply
 */
void PrivateDBusTest::testMethodCallWithPrivateDBus()
{
    QDBusConnection conn = DBusHelper::sessionBus();
    QVERIFY2(conn.isConnected(), "DBus not connected");

    /*
    dbus-send --session           \
        --dest=org.freedesktop.DBus \
        --type=method_call          \
        --print-reply               \
        /org/freedesktop/DBus       \
        org.freedesktop.DBus.ListNames
    */
    QDBusMessage msg = conn.call(
        QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.DBus"),     // Service 
            QStringLiteral("/org/freedesktop/DBus"),    // Path
            QStringLiteral("org.freedesktop.DBus"),     // Interface
            QStringLiteral("ListNames")                 // Method
        )
    );

    QVERIFY2(msg.type() == QDBusMessage::ReplyMessage, "Failed calling method on private DBus");
}

QTEST_MAIN(PrivateDBusTest);
#include "testprivatedbus.moc"
