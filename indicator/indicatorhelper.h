/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef INDICATORHELPER_H
#define INDICATORHELPER_H

#include <QProcess>
#include <QSplashScreen>

#ifdef QSYSTRAY
#include <QSystemTrayIcon>
#else
#include <KStatusNotifierItem>
#endif

class IndicatorHelper
{
public:
    IndicatorHelper();
    ~IndicatorHelper();

    void preInit();
    void postInit();

    void iconPathHook();

    int daemonHook(QProcess &kdeconnectd);

#ifdef QSYSTRAY
    void systrayIconHook(QSystemTrayIcon &systray);
#else
    void systrayIconHook(KStatusNotifierItem &systray);
#endif

private:
#ifdef Q_OS_MAC
    QSplashScreen *m_splashScreen;
#endif
};

#endif
