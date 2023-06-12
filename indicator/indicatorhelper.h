/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef INDICATORHELPER_H
#define INDICATORHELPER_H

#include <QProcess>
#include <QSplashScreen>

#ifdef Q_OS_WIN
#include <QSystemTrayIcon>
#else
#include <KStatusNotifierItem>
#endif

#ifdef Q_OS_WIN
#include <QUrl>
namespace processes
{
const QString dbus_daemon = QStringLiteral("dbus-daemon.exe");
const QString kdeconnect_daemon = QStringLiteral("kdeconnectd.exe");
const QString kdeconnect_app = QStringLiteral("kdeconnect-app.exe");
const QString kdeconnect_handler = QStringLiteral("kdeconnect-handler.exe");
const QString kdeconnect_settings = QStringLiteral("kdeconnect-settings.exe");
const QString kdeconnect_sms = QStringLiteral("kdeconnect-sms.exe");
};
#endif

class IndicatorHelper
{
public:
#ifdef Q_OS_WIN
    IndicatorHelper(const QUrl &indicatorUrl);
#else
    IndicatorHelper();
#endif
    ~IndicatorHelper();

    void preInit();
    void postInit();

    void iconPathHook();

    int daemonHook(QProcess &kdeconnectd);

#ifdef Q_OS_WIN
    void systrayIconHook(QSystemTrayIcon &systray);
#else
    void systrayIconHook(KStatusNotifierItem &systray);
#endif

private:
#ifdef Q_OS_MAC
    QSplashScreen *m_splashScreen;
#endif

#ifdef Q_OS_WIN
    /**
     * Terminate processes of KDE Connect like kdeconnectd.exe and dbus-daemon.exe
     *
     * @return True if termination was successful, false otherwise
     */

    bool terminateProcess(const QString &processName, const QUrl &indicatorUrl) const;
    QUrl m_indicatorUrl;
#endif
};

#endif
