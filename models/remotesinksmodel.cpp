/**
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "remotesinksmodel.h"
#include "models_debug.h"

#include <QDebug>

#include <dbushelper.h>

RemoteSinksModel::RemoteSinksModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(nullptr)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &RemoteSinksModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &RemoteSinksModel::rowsChanged);

    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(DaemonDbusInterface::activatedService(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &RemoteSinksModel::refreshSinkList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &RemoteSinksModel::refreshSinkList);
}

QHash<int, QByteArray> RemoteSinksModel::roleNames() const
{
    // Role names for QML
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(NameRole, "name");
    names.insert(DescriptionRole, "description");
    names.insert(MaxVolumeRole, "maxVolume");
    names.insert(VolumeRole, "volume");
    names.insert(MutedRole, "muted");

    return names;
}

RemoteSinksModel::~RemoteSinksModel()
{
}

QString RemoteSinksModel::deviceId() const
{
    return m_deviceId;
}

void RemoteSinksModel::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) {
        delete m_dbusInterface;
    }

    m_dbusInterface = new RemoteSystemVolumeDbusInterface(deviceId, this);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceRemotesystemvolumeInterface::sinksChanged, this, &RemoteSinksModel::refreshSinkList);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceRemotesystemvolumeInterface::volumeChanged, this, [this](const QString &name, int volume) {
        auto iter = std::find_if(m_sinkList.begin(), m_sinkList.end(), [&name](const Sink &s) {
            return s.name == name;
        });
        if (iter != m_sinkList.end()) {
            iter->volume = volume;
            int i = std::distance(m_sinkList.begin(), iter);
            Q_EMIT dataChanged(index(i, 0), index(i, 0), {VolumeRole});
        }
    });

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceRemotesystemvolumeInterface::mutedChanged, this, [this](const QString &name, bool muted) {
        auto iter = std::find_if(m_sinkList.begin(), m_sinkList.end(), [&name](const Sink &s) {
            return s.name == name;
        });
        if (iter != m_sinkList.cend()) {
            iter->muted = muted;
            int i = std::distance(m_sinkList.begin(), iter);
            Q_EMIT dataChanged(index(i, 0), index(i, 0), {MutedRole});
        }
    });

    refreshSinkList();

    Q_EMIT deviceIdChanged(deviceId);
}

void RemoteSinksModel::refreshSinkList()
{
    if (!m_dbusInterface) {
        return;
    }

    if (!m_dbusInterface->isValid()) {
        qCWarning(KDECONNECT_MODELS) << "dbus interface not valid";
        return;
    }

    beginResetModel();
    m_sinkList.clear();

    const auto cmds = QJsonDocument::fromJson(m_dbusInterface->sinks()).array();
    for (const QJsonValue &cmd : cmds) {
        const QJsonObject cont = cmd.toObject();
        Sink sink;
        sink.name = cont.value(QStringLiteral("name")).toString();
        sink.description = cont.value(QStringLiteral("description")).toString();
        sink.maxVolume = cont.value(QStringLiteral("maxVolume")).toInt();
        sink.volume = cont.value(QStringLiteral("volume")).toInt();
        sink.muted = cont.value(QStringLiteral("muted")).toBool();
        m_sinkList.append(sink);
    }

    endResetModel();
}

QVariant RemoteSinksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_sinkList.count()) {
        return QVariant();
    }

    if (!m_dbusInterface || !m_dbusInterface->isValid()) {
        return QVariant();
    }

    const Sink &sink = m_sinkList[index.row()];

    switch (role) {
    case NameRole:
        return sink.name;
    case DescriptionRole:
        return sink.description;
    case MaxVolumeRole:
        return sink.maxVolume;
    case VolumeRole:
        return sink.volume;
    case MutedRole:
        return sink.muted;
    default:
        return QVariant();
    }
}

bool RemoteSinksModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_sinkList.count()) {
        return false;
    }

    if (!m_dbusInterface || !m_dbusInterface->isValid()) {
        return false;
    }

    const QString sinkName = m_sinkList[index.row()].name;
    switch (role) {
    case VolumeRole:
        m_dbusInterface->sendVolume(sinkName, value.toInt());
        return true;
    case MutedRole:
        m_dbusInterface->sendMuted(sinkName, value.toBool());
        return true;
    default:
        return false;
    }
}

int RemoteSinksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_sinkList.count();
}

#include "moc_remotesinksmodel.cpp"
