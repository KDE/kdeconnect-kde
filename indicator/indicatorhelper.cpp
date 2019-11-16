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

#include "indicatorhelper.h"

IndicatorHelper::IndicatorHelper() {}
IndicatorHelper::~IndicatorHelper() {}

void IndicatorHelper::preInit() {}

void IndicatorHelper::postInit() {}

void IndicatorHelper::iconPathHook() {}

void IndicatorHelper::dbusHook() {}

void IndicatorHelper::qSystemTrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon::fromTheme(QStringLiteral("kdeconnectindicatordark")));
}

void IndicatorHelper::kStatusNotifierItemHook(KStatusNotifierItem &systray)
{
    systray.setIconByName(QStringLiteral("kdeconnectindicatordark"));
}


MacOSIndicatorHelper::MacOSIndicatorHelper()
{
    QIcon kdeconnectIcon = QIcon::fromTheme(QStringLiteral("kdeconnect"));
    QPixmap splashPixmap(kdeconnectIcon.pixmap(256, 256));
    m_splashScreen = new QSplashScreen(splashPixmap);

    m_splashScreen->showMessage(i18n("Launching") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    m_splashScreen->show();
}

MacOSIndicatorHelper::~MacOSIndicatorHelper() {}

void MacOSIndicatorHelper::preInit() {}

void MacOSIndicatorHelper::postInit() {}

void MacOSIndicatorHelper::iconPathHook()
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        QStringList themeSearchPaths = QIcon::themeSearchPaths();
        themeSearchPaths << iconPath;
        QIcon::setThemeSearchPaths(themeSearchPaths);
    }
}

void MacOSIndicatorHelper::dbusHook()
{

    // Unset launchctl env, avoid block
    DBusHelper::macosUnsetLaunchctlEnv();

    // Start kdeconnectd
    m_splashScreen->showMessage(i18n("Launching daemon") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    QProcess kdeconnectdProcess;
    if (QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"))) {
        kdeconnectdProcess.startDetached(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"));
    } else if (QFile::exists(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"))) {
        kdeconnectdProcess.startDetached(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"));
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

            return -1;
        } else {
            QThread::sleep(3);  // Retry after 3s
            retry++;
        }
    } while(true);

    m_splashScreen->showMessage(i18n("Loading modules") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
}

void MacOSIndicatorHelper::kStatusNotifierItemHook(KStatusNotifierItem &systray)
{
    if (!iconPath.isNull()) {
        systray.setIconByName(QStringLiteral("kdeconnectindicatordark"));
    } else {
        // We are in macOS dev env, just continue
        qWarning() << "Fail to find indicator icon, continue anyway";
    }
}

WindowsIndicatorHelper::WindowsIndicatorHelper() {}
WindowsIndicatorHelper::~WindowsIndicatorHelper() {}

void WindowsIndicatorHelper::dbusHook()
{
    QProcess kdeconnectd;
    kdeconnectd.start(QStringLiteral("kdeconnectd.exe"));
}

void WindowsIndicatorHelper::qSystemTrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnectindicatorwin.svg"))));
}

