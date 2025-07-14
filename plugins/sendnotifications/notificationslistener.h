/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <optional>

#include <QHash>
#include <QIODevice>
#include <QSharedPointer>

class KdeConnectPlugin;
struct NotifyingApplication;

#define PACKET_TYPE_NOTIFICATION QStringLiteral("kdeconnect.notification")

class NotificationsListener : public QObject
{
    Q_OBJECT

public:
    explicit NotificationsListener(KdeConnectPlugin *aPlugin);
    ~NotificationsListener() override;

protected:
    bool checkApplicationName(const QString &appName, const QString iconName);
    bool checkIsInBlacklist(const QString &appName, const QString &content);
    QSharedPointer<QIODevice> iconFromQImage(const QImage &image) const;

    KdeConnectPlugin *m_plugin;

private Q_SLOTS:
    void loadApplications();

private:
    void setTranslatedAppName();

    QHash<QString, NotifyingApplication> m_applications;
    QString m_translatedAppName;
};
