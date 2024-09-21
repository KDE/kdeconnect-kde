/**
 * SPDX-FileCopyrightText: 2014 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "dbushelper.h"
#include "core_debug.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

#include "kdeconnectconfig.h"

#ifdef Q_OS_MAC
#include <KLocalizedString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStandardPaths>
#include <QThread>
#endif

namespace DBusHelper
{

void filterNonExportableCharacters(QString &s)
{
    static QRegularExpression regexp(QStringLiteral("[^A-Za-z0-9_]"), QRegularExpression::CaseInsensitiveOption);
    s.replace(regexp, QLatin1String("_"));
}

#ifdef Q_OS_MAC

const QString PrivateDbusAddr = QStringLiteral("unix:tmpdir=/tmp");
const QString PrivateDBusAddressPath = QStringLiteral("/tmp/private_dbus_address");
const QString DBUS_LAUNCHD_SESSION_BUS_SOCKET = QStringLiteral("DBUS_LAUNCHD_SESSION_BUS_SOCKET");

QProcess *m_dbusProcess = nullptr;

void setLaunchctlEnv(const QString &env)
{
    QProcess setLaunchdDBusEnv;
    setLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
    setLaunchdDBusEnv.setArguments({QStringLiteral("setenv"), DBUS_LAUNCHD_SESSION_BUS_SOCKET, env});
    setLaunchdDBusEnv.start();
    setLaunchdDBusEnv.waitForFinished();
}

void unsetLaunchctlEnv()
{
    QProcess unsetLaunchdDBusEnv;
    unsetLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
    unsetLaunchdDBusEnv.setArguments({QStringLiteral("unsetenv"), DBUS_LAUNCHD_SESSION_BUS_SOCKET});
    unsetLaunchdDBusEnv.start();
    unsetLaunchdDBusEnv.waitForFinished();
}

void stopDBusDaemon()
{
    if (m_dbusProcess != nullptr) {
        m_dbusProcess->terminate();
        m_dbusProcess->waitForFinished();
        delete m_dbusProcess;
        m_dbusProcess = nullptr;
        unsetLaunchctlEnv();
    }
}

int startDBusDaemon()
{
    if (m_dbusProcess) {
        return 0;
    }

    qAddPostRoutine(stopDBusDaemon);

    // Unset launchctl env and private dbus addr file, avoid block
    unsetLaunchctlEnv();
    QFile privateDBusAddressFile(PrivateDBusAddressPath);
    if (privateDBusAddressFile.exists()) {
        privateDBusAddressFile.resize(0);
    }

    QString kdeconnectDBusConfiguration;
    QString dbusDaemonExecutable = QStandardPaths::findExecutable(QStringLiteral("dbus-daemon"), {QCoreApplication::applicationDirPath()});
    if (!dbusDaemonExecutable.isNull()) {
        kdeconnectDBusConfiguration = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("dbus-1/session.conf"));
    } else {
        // macOS Debug env
        dbusDaemonExecutable = QLatin1String(qgetenv("craftRoot")) + QLatin1String("/../bin/dbus-daemon");
        kdeconnectDBusConfiguration = QLatin1String(qgetenv("craftRoot")) + QLatin1String("/../share/dbus-1/session.conf");
    }
    m_dbusProcess = new QProcess();
    m_dbusProcess->setProgram(dbusDaemonExecutable);
    m_dbusProcess->setArguments({QStringLiteral("--print-address"),
                                 QStringLiteral("--nofork"),
                                 QStringLiteral("--config-file"),
                                 kdeconnectDBusConfiguration,
                                 QStringLiteral("--address"),
                                 PrivateDbusAddr});
    m_dbusProcess->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_dbusProcess->setStandardOutputFile(PrivateDBusAddressPath);
    m_dbusProcess->start();
    m_dbusProcess->waitForStarted();

    QFile dbusAddressFile(PrivateDBusAddressPath);

    if (!dbusAddressFile.open(QFile::ReadOnly | QFile::Text)) {
        qCCritical(KDECONNECT_CORE) << "Private DBus enabled but error read private dbus address conf";
        return -1;
    }

    QTextStream in(&dbusAddressFile);

    qCDebug(KDECONNECT_CORE) << "Waiting for private dbus";

    int retry = 0;
    QString addr = in.readLine();
    while (addr.length() == 0 && retry < 150) {
        qCDebug(KDECONNECT_CORE) << "Retry reading private DBus address";
        QThread::msleep(100);
        retry++;
        addr = in.readLine(); // Read until first not empty line
    }

    if (addr.length() == 0) {
        qCCritical(KDECONNECT_CORE) << "Private DBus enabled but read private dbus address failed";
        return -2;
    }

    qCDebug(KDECONNECT_CORE) << "Private dbus address: " << addr;

    QRegularExpressionMatch path;
    if (!addr.contains(QRegularExpression(QStringLiteral("path=(?<path>/tmp/dbus-[A-Za-z0-9]+)")), &path)) {
        qCCritical(KDECONNECT_CORE) << "Fail to parse dbus address";
        return -3;
    }

    QString dbusAddress = path.captured(QStringLiteral("path"));
    qCDebug(KDECONNECT_CORE) << "DBus address: " << dbusAddress;
    if (dbusAddress.isEmpty()) {
        qCCritical(KDECONNECT_CORE) << "Fail to extract dbus address";
        return -4;
    }

    setLaunchctlEnv(dbusAddress);

    if (!QDBusConnection::sessionBus().isConnected()) {
        qCCritical(KDECONNECT_CORE) << "Invalid env:" << dbusAddress;
        return -5;
    }

    qCDebug(KDECONNECT_CORE) << "Private D-Bus daemon launched and connected.";
    return 0;
}

#endif // Q_OS_MAC

} // namespace DBusHelper
