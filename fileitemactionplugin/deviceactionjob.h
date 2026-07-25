/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DEVICEACTIONJOB_H
#define DEVICEACTIONJOB_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUrl>

class QAction;
class QDBusPendingCallWatcher;

struct DeviceInfo {
    QString name;
    QString iconName;
};

class DeviceActionJob : public QObject
{
    Q_OBJECT

public:
    explicit DeviceActionJob(const QList<QUrl> &urls, QObject *parent);

    [[nodiscard]] QList<QAction *> actions() const;

    void start(QObject *actionParent);

Q_SIGNALS:
    void finished(const QList<QAction *> &actions);

private:
    void onPluginReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId);
    void onDeviceNameReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId);
    void onDeviceIconNameReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId);

    void fetchDeviceInfo();
    void finalizeActions();

    QList<QUrl> m_urls;
    QList<QAction *> m_actions;
    QObject *m_actionParent = nullptr;

    int m_pendingCalls = 0;
    QStringList m_supportedDeviceIds;
    QMap<QString, DeviceInfo> m_devices;
};

#endif // DEVICEACTIONJOB_H
