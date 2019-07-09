/**
 * Copyright 2014 Albert Vaca <albertvaka@gmail.com>
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

#include "dbushelper.h"
#include "core_debug.h"

#include <QRegExp>
#include <QProcess>
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QCoreApplication>

#include "kdeconnectconfig.h"

#ifdef Q_OS_MAC
#include <CoreFoundation/CFBundle.h>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#endif

namespace DbusHelper {

#ifdef USE_PRIVATE_DBUS
class DBusInstancePrivate
{
public:
    DBusInstancePrivate();
    ~DBusInstancePrivate();

    void launchDBusDaemon();
    void closeDBusDaemon();
private:
    QProcess *m_dbusProcess;
};

static DBusInstancePrivate dbusInstance;
#endif

void filterNonExportableCharacters(QString& s)
{
    static QRegExp regexp(QStringLiteral("[^A-Za-z0-9_]"), Qt::CaseSensitive, QRegExp::Wildcard);
    s.replace(regexp,QLatin1String("_"));
}

QDBusConnection sessionBus()
{
#ifdef USE_PRIVATE_DBUS
    return QDBusConnection::connectToBus(KdeConnectConfig::instance()->privateDBusAddress(),
        QStringLiteral(KDECONNECT_PRIVATE_DBUS_NAME));
#else
    return QDBusConnection::sessionBus();
#endif
}

#ifdef USE_PRIVATE_DBUS
void launchDBusDaemon()
{
    dbusInstance.launchDBusDaemon();
    qAddPostRoutine(closeDBusDaemon);
}

void closeDBusDaemon()
{
    dbusInstance.closeDBusDaemon();
}

#ifdef Q_OS_MAC
void macosUnsetLaunchctlEnv()
{
    // Unset Launchd env
    QProcess unsetLaunchdDBusEnv;
    unsetLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
    unsetLaunchdDBusEnv.setArguments({
        QStringLiteral("unsetenv"),
        QStringLiteral(KDECONNECT_SESSION_DBUS_LAUNCHD_ENV)
    });
    unsetLaunchdDBusEnv.start();
    unsetLaunchdDBusEnv.waitForFinished();
}
#endif

void DBusInstancePrivate::launchDBusDaemon()
{
    // Kill old dbus daemon
    if (m_dbusProcess != nullptr) closeDBusDaemon();

    // Start dbus daemon
    m_dbusProcess = new QProcess();
    #ifdef Q_OS_MAC
        // On macOS, assuming the executable is in Contents/MacOS
        CFURLRef url = (CFURLRef)CFAutorelease((CFURLRef)CFBundleCopyBundleURL(CFBundleGetMainBundle()));
        QString basePath = QUrl::fromCFURL(url).path();
        QString kdeconnectDBusExecutable = basePath + QStringLiteral("Contents/MacOS/dbus-daemon"),
                kdeconnectDBusConfiguration = basePath + QStringLiteral("Contents/Resources/dbus-1/session.conf");
        qCDebug(KDECONNECT_CORE) << "App package path: " << basePath;

        m_dbusProcess->setProgram(kdeconnectDBusExecutable);
        m_dbusProcess->setArguments({QStringLiteral("--print-address"),
            QStringLiteral("--nofork"),
            QStringLiteral("--config-file=") + kdeconnectDBusConfiguration,
            QStringLiteral("--address=") + QStringLiteral(KDECONNECT_PRIVATE_DBUS_ADDR)});
        m_dbusProcess->setWorkingDirectory(basePath);
    #elif defined(Q_OS_WIN)
        // On Windows
        m_dbusProcess->setProgram(QStringLiteral("dbus-daemon.exe"));
        m_dbusProcess->setArguments({QStringLiteral("--session"),
            QStringLiteral("--print-address"),
            QStringLiteral("--nofork"),
            QStringLiteral("--address=") + QStringLiteral(KDECONNECT_PRIVATE_DBUS_ADDR)});
    #else
        // On Linux or other unix-like system
        m_dbusProcess->setProgram(QStringLiteral("dbus-daemon"));
        m_dbusProcess->setArguments({QStringLiteral("--session"),
            QStringLiteral("--print-address"),
            QStringLiteral("--nofork"),
            QStringLiteral("--address=") + QStringLiteral(KDECONNECT_PRIVATE_DBUS_ADDR)});
    #endif
    m_dbusProcess->setStandardOutputFile(KdeConnectConfig::instance()->privateDBusAddressPath());
    m_dbusProcess->setStandardErrorFile(QProcess::nullDevice());
    m_dbusProcess->start();

#ifdef Q_OS_MAC
    // Set launchctl env
    QString privateDBusAddress = KdeConnectConfig::instance()->privateDBusAddress();
    QRegularExpressionMatch path;
    if (privateDBusAddress.contains(QRegularExpression(
            QStringLiteral("path=(?<path>/tmp/dbus-[A-Za-z0-9]+)")
        ), &path)) {
        qCDebug(KDECONNECT_CORE) << "DBus address: " << path.captured(QStringLiteral("path"));
        QProcess setLaunchdDBusEnv;
        setLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
        setLaunchdDBusEnv.setArguments({
            QStringLiteral("setenv"),
            QStringLiteral(KDECONNECT_SESSION_DBUS_LAUNCHD_ENV),
            path.captured(QStringLiteral("path"))
        });
        setLaunchdDBusEnv.start();
        setLaunchdDBusEnv.waitForFinished();
    } else {
        qCDebug(KDECONNECT_CORE) << "Cannot get dbus address";
    }
#endif
}

void DBusInstancePrivate::closeDBusDaemon()
{
    if (m_dbusProcess != nullptr)
    {
        m_dbusProcess->terminate();
        m_dbusProcess->waitForFinished();
        delete m_dbusProcess;
        m_dbusProcess = nullptr;

        QFile privateDBusAddressFile(KdeConnectConfig::instance()->privateDBusAddressPath());

        if (privateDBusAddressFile.exists()) privateDBusAddressFile.resize(0);

#ifdef Q_OS_MAC
        macosUnsetLaunchctlEnv();
#endif
    }
}

DBusInstancePrivate::DBusInstancePrivate()
    :m_dbusProcess(nullptr){}

DBusInstancePrivate::~DBusInstancePrivate()
{
    closeDBusDaemon();
}
#endif

}
