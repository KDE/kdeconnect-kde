/**
 * SPDX-FileCopyrightText: 2014 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "dbushelper.h"
#include "core_debug.h"

#include <QRegExp>
#include <QProcess>
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>

#include "kdeconnectconfig.h"

#ifdef Q_OS_MAC
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#endif

namespace DBusHelper {

#ifdef Q_OS_MAC
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

#ifdef Q_OS_MAC
void launchDBusDaemon()
{
    dbusInstance.launchDBusDaemon();
    qAddPostRoutine(closeDBusDaemon);
}

void closeDBusDaemon()
{
    dbusInstance.closeDBusDaemon();
}

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

void DBusInstancePrivate::launchDBusDaemon()
{
    // Kill old dbus daemon
    if (m_dbusProcess != nullptr) closeDBusDaemon();

    // Start dbus daemon
    m_dbusProcess = new QProcess();

    QString kdeconnectDBusConfiguration;
    QString dbusDaemonExecutable = QStandardPaths::findExecutable(QStringLiteral("dbus-daemon"), { QCoreApplication::applicationDirPath() });
    if (!dbusDaemonExecutable.isNull()) {
        kdeconnectDBusConfiguration = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("dbus-1/session.conf"));
    } else {
        // macOS Debug env
        dbusDaemonExecutable = QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../bin/dbus-daemon");
        kdeconnectDBusConfiguration = QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../share/dbus-1/session.conf");
    }
    m_dbusProcess->setProgram(dbusDaemonExecutable);
    m_dbusProcess->setArguments({QStringLiteral("--print-address"),
        QStringLiteral("--nofork"),
        QStringLiteral("--config-file=") + kdeconnectDBusConfiguration,
        QStringLiteral("--address=") + QStringLiteral(KDECONNECT_PRIVATE_DBUS_ADDR)
    });
    m_dbusProcess->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_dbusProcess->setStandardOutputFile(KdeConnectConfig::instance().privateDBusAddressPath());
    m_dbusProcess->setStandardErrorFile(QProcess::nullDevice());
    m_dbusProcess->start();

#ifdef Q_OS_MAC
    // Set launchctl env
    QString privateDBusAddress = KdeConnectConfig::instance().privateDBusAddress();
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

        QFile privateDBusAddressFile(KdeConnectConfig::instance().privateDBusAddressPath());

        if (privateDBusAddressFile.exists()) privateDBusAddressFile.resize(0);

        macosUnsetLaunchctlEnv();
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
