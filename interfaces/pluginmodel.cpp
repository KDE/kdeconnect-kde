/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pluginmodel.h"

#include <QDebug>

#include "kcoreaddons_version.h"
#include <KConfigGroup>

PluginModel::PluginModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &PluginModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &PluginModel::rowsChanged);
    m_plugins = KPluginMetaData::findPlugins(QStringLiteral("kdeconnect"), std::not_fn(&KPluginMetaData::isHidden));
}

PluginModel::~PluginModel()
{
}

void PluginModel::setDeviceId(const QString &deviceId)
{
    if (deviceId == m_deviceId)
        return;

    m_deviceId = deviceId;
    DeviceDbusInterface *device = new DeviceDbusInterface(m_deviceId);
    m_config = KSharedConfig::openConfig(device->pluginsConfigFile());

    Q_EMIT deviceIdChanged(deviceId);
}

QString PluginModel::pluginDisplayName(const QString &pluginId) const
{
    return KPluginMetaData::findPluginById(QStringLiteral("kdeconnect"), pluginId).name();
}

QUrl PluginModel::pluginSource(const QString &pluginId) const
{
    const QString configFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kdeconnect/%1_config.qml").arg(pluginId));
    if (configFile.isEmpty())
        return {};

    return QUrl::fromLocalFile(configFile);
}

QVariant PluginModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const KPluginMetaData &pluginEntry = m_plugins[index.row()];

    switch (role) {
    case Qt::CheckStateRole: {
        const QString def = pluginEntry.isEnabledByDefault() ? QStringLiteral("true") : QStringLiteral("false");
        return m_config->group(QStringLiteral("Plugins")).readEntry(QStringLiteral("%1Enabled").arg(pluginEntry.pluginId()), def) == QStringLiteral("true");
    }
    case Qt::DisplayRole:
        return pluginEntry.name();
    case IconRole:
        return pluginEntry.iconName();
    case IdRole:
        return pluginEntry.pluginId();
    case ConfigSourceRole:
        return pluginSource(pluginEntry.pluginId());
    case DescriptionRole:
        return pluginEntry.description();
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
    roles[DescriptionRole] = "description";
    return roles;
}

bool PluginModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    bool ret = false;

    if (role == Qt::CheckStateRole) {
        const KPluginMetaData &pluginEntry = m_plugins[index.row()];
        m_config->group(QStringLiteral("Plugins")).writeEntry(QStringLiteral("%1Enabled").arg(pluginEntry.pluginId()), value);
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

#include "moc_pluginmodel.cpp"
