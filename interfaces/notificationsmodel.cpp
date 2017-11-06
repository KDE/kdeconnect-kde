/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "notificationsmodel.h"
#include "interfaces_debug.h"

#include <QDebug>
#include <QDBusInterface>

#include <KSharedConfig>
#include <QIcon>

//#include "modeltest.h"

NotificationsModel::NotificationsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(nullptr)
{

    //new ModelTest(this, this);

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &NotificationsModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &NotificationsModel::rowsChanged);

    connect(this, &QAbstractItemModel::dataChanged,
            this, &NotificationsModel::anyDismissableChanged);
    connect(this, &QAbstractItemModel::rowsInserted,
            this, &NotificationsModel::anyDismissableChanged);

    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(DaemonDbusInterface::activatedService(),
                                                           QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &NotificationsModel::refreshNotificationList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &NotificationsModel::clearNotifications);
}

QHash<int, QByteArray> NotificationsModel::roleNames() const
{
    //Role names for QML
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(DbusInterfaceRole,    "dbusInterface");
    names.insert(AppNameModelRole,     "appName");
    names.insert(IdModelRole,          "notificationId");
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

void NotificationsModel::setDeviceId(const QString& deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) {
        delete m_dbusInterface;
    }

    m_dbusInterface = new DeviceNotificationsDbusInterface(deviceId, this);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::notificationPosted,
            this, &NotificationsModel::notificationAdded);
    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::notificationRemoved,
            this, &NotificationsModel::notificationRemoved);
    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::allNotificationsRemoved,
            this, &NotificationsModel::clearNotifications);
    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceNotificationsInterface::notificationUpdated,
            this, &NotificationsModel::notificationUpdated);

    refreshNotificationList();

    Q_EMIT deviceIdChanged(deviceId);
}

void NotificationsModel::notificationAdded(const QString& id)
{
    int currentSize = m_notificationList.size();
    beginInsertRows(QModelIndex(),  currentSize, currentSize);
    NotificationDbusInterface* dbusInterface = new NotificationDbusInterface(m_deviceId, id, this);
    m_notificationList.append(dbusInterface);
    endInsertRows();
}

void NotificationsModel::notificationRemoved(const QString& id)
{
    for (int i = 0; i < m_notificationList.size(); ++i) {
        if (m_notificationList[i]->notificationId() == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_notificationList.removeAt(i);
            endRemoveRows();
            return;
        }
    }
    qCWarning(KDECONNECT_INTERFACES) << "Attempted to remove unknown notification: " << id;
}

void NotificationsModel::refreshNotificationList()
{
    if (!m_dbusInterface) {
        return;
    }

    clearNotifications();

    if (!m_dbusInterface->isValid()) {
        qCWarning(KDECONNECT_INTERFACES) << "dbus interface not valid";
        return;
    }

    QDBusPendingReply<QStringList> pendingNotificationIds = m_dbusInterface->activeNotifications();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pendingNotificationIds, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, &NotificationsModel::receivedNotifications);
}

void NotificationsModel::receivedNotifications(QDBusPendingCallWatcher* watcher)
{
    watcher->deleteLater();
    clearNotifications();
    QDBusPendingReply<QStringList> pendingNotificationIds = *watcher;

    if (pendingNotificationIds.isError()) {
        qCWarning(KDECONNECT_INTERFACES) << pendingNotificationIds.error();
        return;
    }

    const QStringList notificationIds = pendingNotificationIds.value();
    if (notificationIds.isEmpty()) {
        return;
    }

    beginInsertRows(QModelIndex(), 0, notificationIds.size() - 1);
    for (const QString& notificationId : notificationIds) {
        NotificationDbusInterface* dbusInterface = new NotificationDbusInterface(m_deviceId, notificationId, this);
        m_notificationList.append(dbusInterface);
    }
    endInsertRows();
}

QVariant NotificationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_notificationList.count()
        || !m_notificationList[index.row()]->isValid())
    {
        return QVariant();
    }

    if (!m_dbusInterface || !m_dbusInterface->isValid()) {
        return QVariant();
    }

    NotificationDbusInterface* notification = m_notificationList[index.row()];

    //FIXME: This function gets called lots of times, producing lots of dbus calls. Add a cache?
    switch (role) {
        case IconModelRole:
            return QIcon::fromTheme(QStringLiteral("device-notifier"));
        case IdModelRole:
            return notification->internalId();
        case NameModelRole:
            return notification->ticker();
        case ContentModelRole:
            return QString(); //To implement in the Android side
        case AppNameModelRole:
            return notification->appName();
        case DbusInterfaceRole:
            return qVariantFromValue<QObject*>(notification);
        case DismissableModelRole:
            return notification->dismissable();
        case RepliableModelRole:
            return !notification->replyId().isEmpty();
        case IconPathModelRole:
            return notification->iconPath();
        case TitleModelRole:
            return notification->title();
        case TextModelRole:
            return notification->text();
        default:
             return QVariant();
    }
}

NotificationDbusInterface* NotificationsModel::getNotification(const QModelIndex& index) const
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

int NotificationsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_notificationList.count();
}

bool NotificationsModel::isAnyDimissable() const
{
    for (NotificationDbusInterface* notification : qAsConst(m_notificationList)) {
        if (notification->dismissable()) {
            return true;
        }
    }
    return false;
}

void NotificationsModel::dismissAll()
{
    for (NotificationDbusInterface* notification : qAsConst(m_notificationList)) {
        if (notification->dismissable()) {
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
        endRemoveRows();
    }
}

void NotificationsModel::notificationUpdated(const QString& id)
{
    //TODO only emit the affected indices
    Q_EMIT dataChanged(index(0,0), index(m_notificationList.size() - 1, 0));
}
