/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationsmodel.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QIcon>

#include <algorithm>

#include <dbushelper.h>

#include "models_debug.h"

NotificationsModel::NotificationsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(nullptr)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &NotificationsModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &NotificationsModel::rowsChanged);

    connect(this, &QAbstractItemModel::dataChanged, this, &NotificationsModel::updateAnyDismissable);
    connect(this, &QAbstractItemModel::rowsInserted, this, &NotificationsModel::updateAnyDismissable);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &NotificationsModel::updateAnyDismissable);

    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(DaemonDbusInterface::activatedService(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &NotificationsModel::refreshNotificationList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &NotificationsModel::clearNotifications);
}

QHash<int, QByteArray> NotificationsModel::roleNames() const
{
    // Role names for QML
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(DbusInterfaceRole, "dbusInterface");
    names.insert(AppNameModelRole, "appName");
    names.insert(IdModelRole, "notificationId");
    names.insert(DismissableModelRole, "dismissable");
    names.insert(RepliableModelRole, "repliable");
    names.insert(IconPathModelRole, "appIcon");
    names.insert(TitleModelRole, "title");
    names.insert(TextModelRole, "notitext");
    return names;
}

NotificationsModel::~NotificationsModel()
{
}

QString NotificationsModel::deviceId() const
{
    return m_deviceId;
}

void NotificationsModel::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) {
        delete m_dbusInterface;
    }

    m_dbusInterface = new DeviceNotificationsDbusInterface(deviceId, this);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::notificationPosted, this, &NotificationsModel::notificationAdded);
    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::notificationRemoved, this, &NotificationsModel::notificationRemoved);
    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::allNotificationsRemoved, this, &NotificationsModel::clearNotifications);

    refreshNotificationList();

    Q_EMIT deviceIdChanged(deviceId);
}

NotificationDbusInterface *NotificationsModel::createNotificationInterface(const QString &id)
{
    NotificationDbusInterface *dbusInterface = new NotificationDbusInterface(m_deviceId, id, this);
    connect(dbusInterface, &NotificationDbusInterface::ready, this, &NotificationsModel::notificationUpdated);
    m_data.insert(dbusInterface, NotificationData());
    fetchNotificationData(dbusInterface);
    return dbusInterface;
}

void NotificationsModel::fetchNotificationData(NotificationDbusInterface *notification)
{
    // Fetch all the properties at once asynchronously and cache it
    QDBusMessage msg = QDBusMessage::createMethodCall(notification->service(),
                                                      notification->path(),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("GetAll"));
    msg << notification->interface();
    // Parented to the interface, so it's destroyed (and the reply ignored) if the notification goes away
    auto *watcher = new QDBusPendingCallWatcher(notification->connection().asyncCall(msg), notification);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, notification](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qCWarning(KDECONNECT_MODELS) << "Could not read notification" << notification->notificationId() << reply.error();
            return;
        }
        const int row = m_notificationList.indexOf(notification);
        if (row < 0) {
            return;
        }
        const QVariantMap properties = reply.value();
        NotificationData &data = m_data[notification];
        data.internalId = properties.value(QStringLiteral("internalId")).toString();
        data.appName = properties.value(QStringLiteral("appName")).toString();
        data.ticker = properties.value(QStringLiteral("ticker")).toString();
        data.title = properties.value(QStringLiteral("title")).toString();
        data.text = properties.value(QStringLiteral("text")).toString();
        data.iconPath = properties.value(QStringLiteral("iconPath")).toString();
        data.replyId = properties.value(QStringLiteral("replyId")).toString();
        data.dismissable = properties.value(QStringLiteral("dismissable")).toBool();
        Q_EMIT dataChanged(index(row, 0), index(row, 0));
    });
}

void NotificationsModel::notificationAdded(const QString &id)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_notificationList.prepend(createNotificationInterface(id));
    endInsertRows();
}

void NotificationsModel::notificationRemoved(const QString &id)
{
    for (int i = 0; i < m_notificationList.size(); ++i) {
        if (m_notificationList[i]->notificationId() == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_data.remove(m_notificationList[i]);
            m_notificationList.takeAt(i)->deleteLater();
            endRemoveRows();
            return;
        }
    }
    qCWarning(KDECONNECT_MODELS) << "Attempted to remove unknown notification: " << id;
}

void NotificationsModel::refreshNotificationList()
{
    if (!m_dbusInterface) {
        return;
    }

    clearNotifications();

    if (!m_dbusInterface->isValid()) {
        qCWarning(KDECONNECT_MODELS) << "dbus interface not valid";
        return;
    }

    QDBusPendingReply<QStringList> pendingNotificationIds = m_dbusInterface->activeNotifications();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingNotificationIds, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &NotificationsModel::receivedNotifications);
}

void NotificationsModel::receivedNotifications(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    clearNotifications();
    QDBusPendingReply<QStringList> pendingNotificationIds = *watcher;

    if (pendingNotificationIds.isError()) {
        qCWarning(KDECONNECT_MODELS) << pendingNotificationIds.error();
        return;
    }

    const QStringList notificationIds = pendingNotificationIds.value();
    if (notificationIds.isEmpty()) {
        return;
    }

    beginInsertRows(QModelIndex(), 0, notificationIds.size() - 1);
    for (const QString &notificationId : notificationIds) {
        m_notificationList.append(createNotificationInterface(notificationId));
    }
    endInsertRows();
}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_notificationList.count()) {
        return QVariant();
    }

    NotificationDbusInterface *notification = m_notificationList[index.row()];
    const NotificationData data = m_data.value(notification);

    switch (role) {
    case IconModelRole:
        return QIcon::fromTheme(QStringLiteral("device-notifier"));
    case IdModelRole:
        return data.internalId;
    case NameModelRole:
        return data.ticker;
    case ContentModelRole:
        return QString(); // To implement in the Android side
    case AppNameModelRole:
        return data.appName;
    case DbusInterfaceRole:
        return QVariant::fromValue<QObject *>(notification);
    case DismissableModelRole:
        return data.dismissable;
    case RepliableModelRole:
        return !data.replyId.isEmpty();
    case IconPathModelRole:
        return data.iconPath;
    case TitleModelRole:
        return data.title;
    case TextModelRole:
        return data.text;
    default:
        return QVariant();
    }
}

NotificationDbusInterface *NotificationsModel::getNotification(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    int row = index.row();
    if (row < 0 || row >= m_notificationList.size()) {
        return nullptr;
    }

    return m_notificationList[row];
}

int NotificationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_notificationList.count();
}

bool NotificationsModel::isAnyDismissable() const
{
    return m_anyDismissable;
}

void NotificationsModel::updateAnyDismissable()
{
    const bool anyDismissable = std::any_of(m_data.cbegin(), m_data.cend(), [](const NotificationData &data) {
        return data.dismissable;
    });
    if (anyDismissable != m_anyDismissable) {
        m_anyDismissable = anyDismissable;
        Q_EMIT anyDismissableChanged();
    }
}

void NotificationsModel::dismissAll()
{
    for (NotificationDbusInterface *notification : std::as_const(m_notificationList)) {
        if (m_data.value(notification).dismissable) {
            notification->dismiss();
        }
    }
}

void NotificationsModel::clearNotifications()
{
    if (!m_notificationList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_notificationList.size() - 1);
        qDeleteAll(m_notificationList);
        m_notificationList.clear();
        m_data.clear();
        endRemoveRows();
    }
}

void NotificationsModel::notificationUpdated()
{
    NotificationDbusInterface *notification = qobject_cast<NotificationDbusInterface *>(sender());
    if (!notification || !m_notificationList.contains(notification)) {
        return;
    }
    fetchNotificationData(notification);
}

#include "moc_notificationsmodel.cpp"
