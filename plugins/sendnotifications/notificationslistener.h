/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QBuffer>
#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QFile>
#include <core/device.h>

#include <gio/gio.h>

class KdeConnectPlugin;
class Notification;
struct NotifyingApplication;

class NotificationsListener : public QObject
{
    Q_OBJECT

public:
    explicit NotificationsListener(KdeConnectPlugin *aPlugin);
    ~NotificationsListener() override;

protected:
    KdeConnectPlugin *m_plugin;
    QHash<QString, NotifyingApplication> m_applications;

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

private Q_SLOTS:
    void loadApplications();

private:
    void setTranslatedAppName();
    QString m_translatedAppName;

    GDBusConnection *m_gdbusConnection = nullptr;
    unsigned m_gdbusFilterId = 0;
};
