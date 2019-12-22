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

#include <QApplication>
#include <QIcon>

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
