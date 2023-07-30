/**
 * SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notificationslistener.h"

#include <gio/gio.h>

class DBusNotificationsListener : public NotificationsListener
{
    Q_OBJECT

public:
    explicit DBusNotificationsListener(KdeConnectPlugin *aPlugin);
    ~DBusNotificationsListener() override;

private:
    void setupDBusMonitor();

    // virtual helper function to make testing possible (QDBusArgument can not
    // be injected without making a DBUS-call):
    virtual bool parseImageDataArgument(GVariant *argument,
                                        int &width,
                                        int &height,
                                        int &rowStride,
                                        int &bitsPerSample,
                                        int &channels,
                                        bool &hasAlpha,
                                        QByteArray &imageData) const;
    QSharedPointer<QIODevice> iconForImageData(GVariant *argument) const;
    static QSharedPointer<QIODevice> iconForIconName(const QString &iconName);

    static GDBusMessage *onMessageFiltered(GDBusConnection *connection, GDBusMessage *msg, int incoming, void *parent);

    GDBusConnection *m_gdbusConnection = nullptr;
    unsigned m_gdbusFilterId = 0;
};
