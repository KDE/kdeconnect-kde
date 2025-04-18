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

IndicatorHelper::IndicatorHelper()
    : m_indicatorUrl(QUrl::fromLocalFile(QApplication::applicationDirPath()))
{
    uiSettings = winrt::Windows::UI::ViewManagement::UISettings();
}

IndicatorHelper::~IndicatorHelper()
{
    this->terminateProcess(processes::dbus_daemon, m_indicatorUrl);
    this->terminateProcess(processes::kdeconnect_app, m_indicatorUrl);
    this->terminateProcess(processes::kdeconnect_handler, m_indicatorUrl);
    this->terminateProcess(processes::kdeconnect_sms, m_indicatorUrl);
    this->terminateProcess(processes::kdeconnect_daemon, m_indicatorUrl);
}

void IndicatorHelper::iconPathHook()
{
}

int IndicatorHelper::startDaemon()
{
    kdeconnectd.start(processes::kdeconnect_daemon, QStringList());
    return 0;
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

bool IndicatorHelper::terminateProcess(const QString &processName, const QUrl &indicatorUrl) const
{
    HANDLE hProcessSnap;
    HANDLE hProcess;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qCWarning(KDECONNECT_INDICATOR) << "Failed to get snapshot of processes.";
        return FALSE;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        qCWarning(KDECONNECT_INDICATOR) << "Failed to get handle for the first process.";
        CloseHandle(hProcessSnap);
        return FALSE;
    }

    do {
        if (QString::fromWCharArray((wchar_t *)pe32.szExeFile) == processName) {
            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

            if (hProcess == NULL) {
                qCWarning(KDECONNECT_INDICATOR) << "Failed to get handle for the process:" << processName;
                return FALSE;
            } else {
                const DWORD processPathSize = 4096;
                CHAR processPathString[processPathSize];

                BOOL gotProcessPath = QueryFullProcessImageNameA(hProcess, 0, (LPSTR)processPathString, (PDWORD)&processPathSize);

                if (gotProcessPath) {
                    const QUrl processUrl = QUrl::fromLocalFile(QString::fromStdString(processPathString)); // to replace \\ with /
                    if (indicatorUrl.isParentOf(processUrl)) {
                        BOOL terminateSuccess = TerminateProcess(hProcess, 0);
                        if (!terminateSuccess) {
                            qCWarning(KDECONNECT_INDICATOR) << "Failed to terminate process:" << processName;
                            return FALSE;
                        }
                    }
                }
            }
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return TRUE;
}
