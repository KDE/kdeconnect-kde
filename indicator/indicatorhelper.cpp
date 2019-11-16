/*
 * Copyright 2019 Weixuan XIAO <veyx.shaw@gmail.com>
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

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>
#include <QThread>
#include <QDebug>

#include <KLocalizedString>

#include <dbushelper.h>

#include "indicatorhelper.h"

IndicatorHelper::IndicatorHelper() {}
IndicatorHelper::~IndicatorHelper() {}

void IndicatorHelper::preInit() {}

void IndicatorHelper::postInit() {}

void IndicatorHelper::iconPathHook() {}

int IndicatorHelper::daemonHook(QProcess &kdeconnectd)
{
    Q_UNUSED(kdeconnectd);

    return 0;
}

#ifdef QSYSTRAY
void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon::fromTheme(QStringLiteral("kdeconnectindicatordark")));
}
#else
void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    systray.setIconByName(QStringLiteral("kdeconnectindicatordark"));
}
#endif

MacOSIndicatorHelper::MacOSIndicatorHelper()
{
    QIcon kdeconnectIcon = QIcon::fromTheme(QStringLiteral("kdeconnect"));
    QPixmap splashPixmap(kdeconnectIcon.pixmap(256, 256));
    m_splashScreen = new QSplashScreen(splashPixmap);

    m_splashScreen->showMessage(i18n("Launching") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    m_splashScreen->show();
}

MacOSIndicatorHelper::~MacOSIndicatorHelper()
{
    if (m_splashScreen) {
        delete m_splashScreen;
    }
}

void MacOSIndicatorHelper::postInit()
{
    if (m_splashScreen) {
        m_splashScreen->finish(nullptr);
    }
}

void MacOSIndicatorHelper::iconPathHook()
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        QStringList themeSearchPaths = QIcon::themeSearchPaths();
        themeSearchPaths << iconPath;
        QIcon::setThemeSearchPaths(themeSearchPaths);
    }
}

int MacOSIndicatorHelper::daemonHook(QProcess &kdeconnectd)
{

    // Unset launchctl env, avoid block
    DBusHelper::macosUnsetLaunchctlEnv();

    // Start kdeconnectd
    m_splashScreen->showMessage(i18n("Launching daemon") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    if (QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"))) {
        kdeconnectd.startDetached(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"));
    } else if (QFile::exists(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"))) {
        kdeconnectd.startDetached(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"));
    } else {
        QMessageBox::critical(nullptr, i18n("KDE Connect"),
                              i18n("Cannot find kdeconnectd"),
                              QMessageBox::Abort,
                              QMessageBox::Abort);
        return -1;
    }

    // Wait for dbus daemon env
    QProcess getLaunchdDBusEnv;
    m_splashScreen->showMessage(i18n("Waiting D-Bus") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    int retry = 0;
    do {
        getLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
        getLaunchdDBusEnv.setArguments({
            QStringLiteral("getenv"),
            QStringLiteral(KDECONNECT_SESSION_DBUS_LAUNCHD_ENV)
        });
        getLaunchdDBusEnv.start();
        getLaunchdDBusEnv.waitForFinished();

        QString launchdDBusEnv = QString::fromLocal8Bit(getLaunchdDBusEnv.readAllStandardOutput());

        if (launchdDBusEnv.length() > 0) {
            break;
        } else if (retry >= 10) {
            // Show a warning and exit
            qCritical() << "Fail to get launchctl" << KDECONNECT_SESSION_DBUS_LAUNCHD_ENV << "env";

            QMessageBox::critical(nullptr, i18n("KDE Connect"),
                                  i18n("Cannot connect to DBus\n"
                                  "KDE Connect will quit"),
                                  QMessageBox::Abort,
                                  QMessageBox::Abort);
            return -2;
        } else {
            QThread::sleep(3);  // Retry after 3s
            retry++;
        }
    } while(true);

    m_splashScreen->showMessage(i18n("Loading modules") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);

    return 0;
}

#ifdef QSYSTRAY
void MacOSIndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    Q_UNUSED(systray);
}
#else
void MacOSIndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        systray.setIconByName(QStringLiteral("kdeconnectindicatordark"));
    } else {
        // We are in macOS dev env, just continue
        qWarning() << "Fail to find indicator icon, continue anyway";
    }
}
#endif

WindowsIndicatorHelper::WindowsIndicatorHelper() {}
WindowsIndicatorHelper::~WindowsIndicatorHelper() {}

int WindowsIndicatorHelper::daemonHook(QProcess &kdeconnectd)
{
    kdeconnectd.start(QStringLiteral("kdeconnectd.exe"));
    return 0;
}

#ifdef QSYSTRAY
void WindowsIndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnectindicatorwin.svg"))));
}
#else
void WindowsIndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    Q_UNUSED(systray);
}
#endif
