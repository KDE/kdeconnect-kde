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

DesktopDaemon::DesktopDaemon(QObject *parent)
    : Daemon(parent)
{
    qApp->setWindowIcon(QIcon(QStringLiteral(":/icons/kdeconnect/kdeconnect.png")));
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
