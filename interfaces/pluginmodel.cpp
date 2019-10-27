/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pluginmodel.h"

#include <QDebug>
#include <KPluginMetaData>

PluginModel::PluginModel(QObject *parent)
    : QAbstractListModel(parent)
{

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &PluginModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &PluginModel::rowsChanged);


    beginResetModel();
    m_plugins = KPluginInfo::fromMetaData(KPluginLoader::findPlugins(QStringLiteral("kdeconnect/")));
    endResetModel();
}

PluginModel::~PluginModel()
{
}

void PluginModel::setDeviceId(const QString& deviceId)
{
    if (deviceId == m_deviceId)
        return;

    m_deviceId = deviceId;
    DeviceDbusInterface *device = new DeviceDbusInterface(m_deviceId);
    m_config = KSharedConfig::openConfig(device->pluginsConfigFile());
}

QVariant PluginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const KPluginInfo &pluginEntry = m_plugins[index.row()];

    switch (role) {
    case Qt::CheckStateRole:
    {
        QString def = pluginEntry.isPluginEnabledByDefault() ? QStringLiteral("true") : QStringLiteral("false");
        return m_config->group("Plugins").readEntry(QStringLiteral("%1Enabled").arg(pluginEntry.pluginName()), def) == QStringLiteral("true");
    }
    case Qt::DisplayRole:
        return pluginEntry.name();
    case IconRole:
        return pluginEntry.icon();
    case IdRole:
        return pluginEntry.pluginName();
    case ConfigSourceRole:
    {
        const QString configFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kdeconnect/%1_config.qml").arg(pluginEntry.pluginName()));
        if (configFile.isEmpty())
            return QUrl();

        return QUrl::fromLocalFile(configFile);
    }
    default:
        return QVariant();
    }
}


QHash<int, QByteArray> PluginModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[Qt::DisplayRole] = "name";
    roles[Qt::CheckStateRole] = "isChecked";
    roles[IconRole] = "iconName";
    roles[IdRole] = "pluginId";
    roles[ConfigSourceRole] = "configSource";
    return roles;
}


bool PluginModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    bool ret = false;

    if (role == Qt::CheckStateRole) {
        const KPluginInfo &pluginEntry = m_plugins[index.row()];
        m_config->group("Plugins").writeEntry(QStringLiteral("%1Enabled").arg(pluginEntry.pluginName()), value);
        ret = true;
    }

    m_config->sync();

    if (ret) {
        Q_EMIT dataChanged(index, index);
    }

    return ret;
}

int PluginModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_plugins.size();
}

QString PluginModel::deviceId()
{
    return m_deviceId;
}
