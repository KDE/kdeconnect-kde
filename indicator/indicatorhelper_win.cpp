/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>

#include <iostream>

#include <Windows.h>
#include <tlhelp32.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "indicator_debug.h"
#include "indicatorhelper.h"
winrt::Windows::UI::ViewManagement::UISettings uiSettings;

#include <QProcess>

IndicatorHelper::IndicatorHelper()
{
    uiSettings = winrt::Windows::UI::ViewManagement::UISettings();
}

IndicatorHelper::~IndicatorHelper()
{
}

void IndicatorHelper::iconPathHook()
{
}

void IndicatorHelper::startDaemon()
{
    QProcess::startDetached(QStringLiteral("kdeconnectd.exe"), QStringList());
}

void onThemeChanged(QSystemTrayIcon &systray)
{
    // Since this is a system tray icon,  we care about the system theme and not the app theme
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::Registry64Format);
    bool isLightTheme = registry.value(QStringLiteral("SystemUsesLightTheme")).toBool();
    if (isLightTheme) {
        systray.setIcon(
            QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnectindicator.svg"))));
    } else {
        systray.setIcon(
            QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnectindicatordark.svg"))));
    }
}

void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    // Set a callback so we can detect changes to light/dark themes and manually call the callback once the first time
    uiSettings.ColorValuesChanged([&systray](auto &&unused1, auto &&unused2) {
        onThemeChanged(systray);
    });
    onThemeChanged(systray);
}
