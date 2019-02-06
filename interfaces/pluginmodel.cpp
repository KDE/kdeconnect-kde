/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
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
    case CheckedRole:
        return m_config->group("Plugins").readEntry(QStringLiteral("%1Enabled").arg(pluginEntry.pluginName()), QStringLiteral("false")) == QStringLiteral("true");
    case NameRole:
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
    roles[NameRole] = "name";
    roles[CheckedRole] = "isChecked";
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

    if (role == CheckedRole) {
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
