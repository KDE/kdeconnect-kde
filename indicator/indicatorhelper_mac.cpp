/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "indicatorhelper.h"

#include <KLocalizedString>
#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QStandardPaths>

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

void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    QString iconPathInBundle = QStringLiteral("kdeconnect-icons/hicolor/scalable/apps/kdeconnectindicator.svg");
    QString iconPath = QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, iconPathInBundle);
    QIcon icon(iconPath);
    icon.setIsMask(true); // Make icon adapt to menu bar color
    systray.setIconByPixmap(icon);
}
