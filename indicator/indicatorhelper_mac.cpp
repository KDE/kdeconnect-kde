/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "indicatorhelper.h"

#include <KLocalizedString>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>

#include "serviceregister_mac.h"
#include <dbushelper.h>
#include <kdeconnectconfig.h>

IndicatorHelper::IndicatorHelper()
{
    registerServices();
}

IndicatorHelper::~IndicatorHelper()
{
}

void IndicatorHelper::iconPathHook()
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("kdeconnect-icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        QStringList themeSearchPaths = QIcon::themeSearchPaths();
        themeSearchPaths << iconPath;
        QIcon::setThemeSearchPaths(themeSearchPaths);
    }
}

void IndicatorHelper::startDaemon()
{
    int dbusStatus = DBusHelper::startDBusDaemon();
    if (dbusStatus) {
        QMessageBox::critical(nullptr,
                              i18n("KDE Connect"),
                              i18n("Cannot connect to DBus\n"
                                   "KDE Connect will quit"),
                              QMessageBox::Abort,
                              QMessageBox::Abort);
        QApplication::exit(dbusStatus);
    }

    // Start kdeconnectd, the daemon will not duplicate when there is already one
    QString daemonPath = QCoreApplication::applicationDirPath() + QLatin1String("/kdeconnectd");
    if (!QFile::exists(daemonPath)) {
        QMessageBox::critical(nullptr, i18n("KDE Connect"), i18n("Cannot find kdeconnectd"), QMessageBox::Abort, QMessageBox::Abort);
        QApplication::exit(-10);
    }
    QProcess::startDetached(daemonPath);
}

void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("kdeconnect-icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        auto icon = QIcon::fromTheme(QStringLiteral("kdeconnectindicator"));
        icon.setIsMask(true); // Make icon adapt to menu bar color
        systray.setIconByPixmap(icon);
    } else {
        // We are in macOS dev env, just continue
        qWarning() << "Fail to find indicator icon, continue anyway";
    }
}
