/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "deviceactionjob.h"

#include <QAction>
#include <QDBusPendingCallWatcher>
#include <QIcon>
#include <QUrl>
#include <QVariantList>
#include <QWidget>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/devicesmodel.h"

#include <dbushelper.h>

#include <algorithm>

#include "kdeconnect_fileitemaction_debug.h"

DeviceActionJob::DeviceActionJob(const QList<QUrl> &urls, QObject *parent)
    : QObject(parent)
    , m_urls(urls)
{
    connect(this, &DeviceActionJob::finished, this, &QObject::deleteLater);
}

QList<QAction *> DeviceActionJob::actions() const
{
    return m_actions;
}

void DeviceActionJob::start(QObject *actionParent)
{
    m_actionParent = actionParent;
    // Must only ever call start() once.
    Q_ASSERT(m_actions.isEmpty());

    DaemonDbusInterface iface;
    auto reply = iface.devices(true, true);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusPendingReply<QStringList> reply = *watcher;
        if (reply.isError()) {
            qCWarning(KDECONNECT_FILEITEMACTION) << "Failed to get list of devices:" << reply.error().message();
            Q_EMIT finished({});
            return;
        }

        const QStringList deviceIds = reply.value();
        for (const QString &deviceId : deviceIds) {
            DeviceDbusInterface deviceIface(deviceId);

            auto pluginReply = deviceIface.hasPlugin(QStringLiteral("kdeconnect_share"));
            ++m_pendingCalls;
            auto *pluginWatcher = new QDBusPendingCallWatcher(pluginReply, this);
            connect(pluginWatcher, &QDBusPendingCallWatcher::finished, this, [this, pluginWatcher, deviceId] {
                onPluginReplyFinished(pluginWatcher, deviceId);
            });
        }
    });
}

void DeviceActionJob::onPluginReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId)
{
    QDBusPendingReply<bool> reply = *watcher;

    if (reply.isError()) {
        qCWarning(KDECONNECT_FILEITEMACTION) << "Failed to get check whether device" << deviceId << "supports plugin:" << reply.error().message();
    } else {
        if (reply.value()) {
            m_supportedDeviceIds.append(deviceId);
        }
    }

    if (--m_pendingCalls == 0) {
        fetchDeviceInfo();
    }
    watcher->deleteLater();
}

void DeviceActionJob::fetchDeviceInfo()
{
    if (m_supportedDeviceIds.isEmpty()) {
        Q_EMIT finished({});
        return;
    }

    const QString propertiesIface = QStringLiteral("org.freedesktop.DBus.Properties");
    const QString getMethod = QStringLiteral("Get");

    for (const QString &deviceId : std::as_const(m_supportedDeviceIds)) {
        DeviceDbusInterface deviceIface(deviceId);

        {
            // Unfortunately no easy built-in async property read on QDBusAbstractInterface.
            auto nameMessage = QDBusMessage::createMethodCall(deviceIface.service(), deviceIface.path(), propertiesIface, getMethod);
            nameMessage << deviceIface.interface() << QStringLiteral("name");

            auto nameReply = deviceIface.connection().asyncCall(nameMessage);
            ++m_pendingCalls;
            auto *nameWatcher = new QDBusPendingCallWatcher(nameReply, this);
            connect(nameWatcher, &QDBusPendingCallWatcher::finished, this, [this, nameWatcher, deviceId] {
                onDeviceNameReplyFinished(nameWatcher, deviceId);
            });
        }

        {
            // Unfortunately no easy built-in async property read on QDBusAbstractInterface.
            auto iconNameMessage = QDBusMessage::createMethodCall(deviceIface.service(), deviceIface.path(), propertiesIface, getMethod);
            iconNameMessage << deviceIface.interface() << QStringLiteral("iconName");

            auto iconNameReply = deviceIface.connection().asyncCall(iconNameMessage);
            ++m_pendingCalls;
            auto *iconNameWatcher = new QDBusPendingCallWatcher(iconNameReply, this);
            connect(iconNameWatcher, &QDBusPendingCallWatcher::finished, this, [this, iconNameWatcher, deviceId] {
                onDeviceIconNameReplyFinished(iconNameWatcher, deviceId);
            });
        }
    }
}

void DeviceActionJob::onDeviceNameReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KDECONNECT_FILEITEMACTION).nospace() << "Failed to get name of" << deviceId << ": " << reply.error().message();
    } else {
        m_devices[deviceId].name = reply.value().variant().toString();
    }

    if (--m_pendingCalls == 0) {
        finalizeActions();
    }
    watcher->deleteLater();
}

void DeviceActionJob::onDeviceIconNameReplyFinished(QDBusPendingCallWatcher *watcher, const QString &deviceId)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KDECONNECT_FILEITEMACTION).nospace() << "Failed to get icon name of" << deviceId << ": " << reply.error().message();
    } else {
        m_devices[deviceId].iconName = reply.value().variant().toString();
    }

    if (--m_pendingCalls == 0) {
        finalizeActions();
    }
    watcher->deleteLater();
}

void DeviceActionJob::finalizeActions()
{
    for (const auto &[deviceId, device] : std::as_const(m_devices).asKeyValueRange()) {
        // Don't offer "Send to" the same device.
        const bool urlIsSameDevice = std::all_of(m_urls.cbegin(), m_urls.cend(), [&deviceId](const QUrl &url) {
            return url.scheme() == QLatin1String("kdeconnect") && url.host() == deviceId;
        });
        if (urlIsSameDevice) {
            continue;
        }

        QAction *action = new QAction(QIcon::fromTheme(device.iconName), device.name, m_actionParent);
        action->setProperty("id", deviceId);
        connect(action, &QAction::triggered, action, [deviceId, urls = m_urls] {
            ShareDbusInterface shareIface(deviceId);
            shareIface.shareUrls(QUrl::toStringList(urls));
        });

        m_actions.append(action);
    }
    Q_EMIT finished(m_actions);
}

#include "moc_deviceactionjob.cpp"
