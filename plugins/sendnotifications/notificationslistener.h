/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2018 Richard Liebscher <richard.liebscher@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QIODevice>
#include <QSharedPointer>
#include <QDBusConnection>

#include "notifyingapplication.h"

class KdeConnectPlugin;

struct NotifyingApplication;

#define PACKET_TYPE_NOTIFICATION QStringLiteral("kdeconnect.notification")

// TODO: make a singleton, shared for all devices
class NotificationsListener : public QObject
{
    Q_OBJECT

public:
    explicit NotificationsListener(KdeConnectPlugin *aPlugin);

protected:
    KdeConnectPlugin *m_plugin;
    QHash<QString, NotifyingApplication> m_applications;

    // virtual helper function to make testing possible (QDBusArgument can not
    // be injected without making a DBUS-call):
    virtual bool parseImageDataArgument(const QVariant &argument,
                                        int &width,
                                        int &height,
                                        int &rowStride,
                                        int &bitsPerSample,
                                        int &channels,
                                        bool &hasAlpha,
                                        QByteArray &imageData) const;
    QSharedPointer<QIODevice> iconForImageData(const QVariant &argument) const;
    QSharedPointer<QIODevice> iconForIconName(const QString &iconName) const;
    QSharedPointer<QIODevice> iconFromQImage(const QImage &image) const;

    void handleNotification(const QString &, uint, const QString &, const QString &, const QString &, const QStringList &, const QVariantMap &, int);

private Q_SLOTS:
    void handleMessage(const QDBusMessage& message);
    void loadApplications();

private:
    QDBusConnection sessionBus;
};
