/**
 * SPDX-FileCopyrightText: 2024 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "desktop_daemon.h"

#include <QApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QTimer>

#include <KDBusService>
#include <KIO/Global>
#include <KIO/JobTracker>
#include <KLocalizedString>
#include <KNotification>

#include "core/backends/pairinghandler.h"
#include "core/device.h"
#include "core/openconfig.h"

#ifdef Q_OS_WIN
static void terminateProcess(const QString &processName, const QUrl &indicatorUrl) const
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
#endif

DesktopDaemon::DesktopDaemon(QObject *parent)
    : Daemon(parent)
{
    qApp->setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
}

DesktopDaemon::~DesktopDaemon()
{
#ifdef Q_OS_WIN
    // TODO: Killing the other apps when the daemon dies seems like a good
    // idea that we probably want to replicate on all platforms (minus dbus-daemon)
    QUrl m_indicatorUrl = QUrl::fromLocalFile(QApplication::applicationDirPath());
    terminateProcess(QStringLiteral("dbus-daemon.exe"), m_indicatorUrl);
    terminateProcess(QStringLiteral("kdeconnect-app.exe"), m_indicatorUrl);
    terminateProcess(QStringLiteral("kdeconnect-handler.exe"), m_indicatorUrl);
    terminateProcess(QStringLiteral("kdeconnect-sms.exe"), m_indicatorUrl);
#endif
}

void DesktopDaemon::askPairingConfirmation(Device *device)
{
    KNotification *notification = new KNotification(QStringLiteral("pairingRequest"), KNotification::NotificationFlag::Persistent);
    QTimer::singleShot(PairingHandler::pairingTimeoutMsec, notification, &KNotification::close);
    notification->setIconName(QStringLiteral("dialog-information"));
    notification->setComponentName(QStringLiteral("kdeconnect"));
    notification->setTitle(QStringLiteral("KDE Connect"));
    notification->setText(i18n("Pairing request from %1\nKey: %2", device->name().toHtmlEscaped(), device->verificationKey()));
    QString deviceId = device->id();
    auto openSettings = [deviceId, notification] {
        OpenConfig oc;
        oc.setXdgActivationToken(notification->xdgActivationToken());
        oc.openConfiguration(deviceId);
    };

    KNotificationAction *openSettingsAction = notification->addDefaultAction(i18n("Open"));
    connect(openSettingsAction, &KNotificationAction::activated, openSettings);

    KNotificationAction *acceptAction = notification->addAction(i18n("Accept"));
    connect(acceptAction, &KNotificationAction::activated, device, &Device::acceptPairing);

    KNotificationAction *rejectAction = notification->addAction(i18n("Reject"));
    connect(rejectAction, &KNotificationAction::activated, device, &Device::cancelPairing);

    notification->sendEvent();
}

void DesktopDaemon::reportError(const QString &title, const QString &description)
{
    qWarning() << title << ":" << description;
    KNotification::event(KNotification::Error, title, description);
}

KJobTrackerInterface *DesktopDaemon::jobTracker()
{
    return KIO::getJobTracker();
}

Q_SCRIPTABLE void DesktopDaemon::sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName)
{
    KNotification *notification = new KNotification(eventId); // KNotification::Persistent
    notification->setIconName(iconName);
    notification->setComponentName(QStringLiteral("kdeconnect"));
    notification->setTitle(title);
    notification->setText(text);
    notification->sendEvent();
}

void DesktopDaemon::quit()
{
    QApplication::quit();
}
