/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2018 Richard Liebscher <richard.liebscher@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notificationslistener.h"

#include "pw_libnotificationmanager/notification.h"

#include <atomic>

#include <QThread>

#include <dbus/dbus.h>

class DBusNotificationsListenerThread : public QThread
{
    Q_OBJECT

public:
    void run() override;
    void stop();
    void handleNotifyCall(DBusMessage *message);

Q_SIGNALS:
    void notificationReceived(const NotificationManager::Notification &);

private:
    std::atomic<DBusConnection *> m_connection = nullptr;
};

class DBusNotificationsListener : public NotificationsListener
{
    Q_OBJECT

public:
    explicit DBusNotificationsListener(KdeConnectPlugin *aPlugin);
    ~DBusNotificationsListener() override;

private:
    void onNotify(const NotificationManager::Notification &);

    bool parseImageDataArgument(const QVariant &argument,
                                int &width,
                                int &height,
                                int &rowStride,
                                int &bitsPerSample,
                                int &channels,
                                bool &hasAlpha,
                                QByteArray &imageData) const;
    QSharedPointer<QIODevice> iconForImageData(const QVariant &argument) const;
    QSharedPointer<QIODevice> iconForIconName(const QString &iconName) const;
    QSharedPointer<QIODevice> pngFromImage() const;

    DBusNotificationsListenerThread *m_thread = nullptr;
};
