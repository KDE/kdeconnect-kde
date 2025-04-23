/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "indicatorhelper.h"

#include <QIcon>

IndicatorHelper::IndicatorHelper()
{
}

IndicatorHelper::~IndicatorHelper()
{
}

void IndicatorHelper::iconPathHook()
{
}

int IndicatorHelper::startDaemon()
{
    return 0;
}

void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    // Assume we are on Gnome, where the system bar is always black regardless of the theme
    systray.setIcon(QIcon::fromTheme(QStringLiteral("kdeconnectindicatordark")));
}
