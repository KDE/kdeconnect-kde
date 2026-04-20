/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef INDICATORHELPER_H
#define INDICATORHELPER_H

#ifdef Q_OS_WIN
#include <QSystemTrayIcon>
#else
#include <KStatusNotifierItem>
#endif

class IndicatorHelper
{
public:
    IndicatorHelper();
    ~IndicatorHelper();

    void iconPathHook();

    void startDaemon();

#ifdef Q_OS_WIN
    void systrayIconHook(QSystemTrayIcon &systray);
#else
    void systrayIconHook(KStatusNotifierItem &systray);
#endif
};

#endif
