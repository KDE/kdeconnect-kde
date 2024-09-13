/**
 * SPDX-FileCopyrightText: 2024 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QString>

#include "daemon.h"

class Device;
class KJobTrackerInterface;

class DesktopDaemon : public Daemon
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
public:
    DesktopDaemon(QObject *parent = nullptr);

    void askPairingConfirmation(Device *device) override;
    void reportError(const QString &title, const QString &description) override;
    KJobTrackerInterface *jobTracker() override;
    Q_SCRIPTABLE void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) override;
    void quit() override;
};
