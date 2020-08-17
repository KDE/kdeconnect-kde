/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QFile>
#include <QIcon>
#include <QStandardPaths>

#include "indicatorhelper.h"

IndicatorHelper::IndicatorHelper() {}
IndicatorHelper::~IndicatorHelper() {}

void IndicatorHelper::preInit() {}

void IndicatorHelper::postInit() {}

void IndicatorHelper::iconPathHook() {}

int IndicatorHelper::daemonHook(QProcess &kdeconnectd)
{
    kdeconnectd.start(QStringLiteral("kdeconnectd.exe"));
    return 0;
}

#ifdef QSYSTRAY
void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnectindicatorwin.svg"))));
}
#else
void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    Q_UNUSED(systray);
}
#endif
