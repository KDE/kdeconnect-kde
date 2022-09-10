/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NOTIFICATIONSLISTENER_H
#define NOTIFICATIONSLISTENER_H

#include <QBuffer>
#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QFile>
#include <core/device.h>

class KdeConnectPlugin;
class Notification;
struct NotifyingApplication;

class NotificationsListener : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit NotificationsListener(KdeConnectPlugin *aPlugin);
    ~NotificationsListener() override;

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

public Q_SLOTS:
    Q_SCRIPTABLE uint Notify(const QString &, uint, const QString &, const QString &, const QString &, const QStringList &, const QVariantMap &, int);

private Q_SLOTS:
    void loadApplications();

private:
    void setTranslatedAppName();
    QString m_translatedAppName;
};

#endif // NOTIFICATIONSLISTENER_H
