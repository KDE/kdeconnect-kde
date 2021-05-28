/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QFile>
#include <QIcon>
#include <QStandardPaths>
#include <QDebug>

#include <iostream>

#include <Windows.h>
#include <tlhelp32.h>

#include "indicatorhelper.h"
#include "indicator_debug.h"

IndicatorHelper::IndicatorHelper(const QUrl& indicatorUrl)
    : m_indicatorUrl(indicatorUrl) {}

IndicatorHelper::~IndicatorHelper() {
    this->terminateProcess(processes::dbus_daemon, m_indicatorUrl);
    this->terminateProcess(processes::kdeconnect_daemon, m_indicatorUrl);
}

void IndicatorHelper::preInit() {}

void IndicatorHelper::postInit() {}

void IndicatorHelper::iconPathHook() {}

int IndicatorHelper::daemonHook(QProcess &kdeconnectd)
{
    kdeconnectd.start(processes::kdeconnect_daemon);
    return 0;
}

#ifdef QSYSTRAY
void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    systray.setIcon(QIcon::fromTheme(QStringLiteral("kdeconnect-tray")));
}
#else
void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    Q_UNUSED(systray);
}
#endif


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

    do
    {
        if (QString::fromWCharArray((wchar_t *)pe32.szExeFile) == processName) {
            hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

            if (hProcess == NULL) {
                qCWarning(KDECONNECT_INDICATOR) << "Failed to get handle for the process:" << processName;
                return FALSE;
            } else {
                const DWORD processPathSize = 4096;
                CHAR processPathString[processPathSize];

                BOOL gotProcessPath = QueryFullProcessImageNameA(
                                hProcess,
                                0,
                                (LPSTR)processPathString,
                                (PDWORD) &processPathSize
                            );

                if (gotProcessPath) {
                    const QUrl processUrl = QUrl::fromLocalFile(QString::fromStdString(processPathString)); // to replace \\ with /
                    if (indicatorUrl.isParentOf(processUrl)) {
                        BOOL terminateSuccess = TerminateProcess(
                                                    hProcess,
                                                    0
                                                );
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

