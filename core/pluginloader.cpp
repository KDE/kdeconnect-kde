/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pluginloader.h"

#include <KPluginFactory>
#include <KPluginMetaData>
#include <QPluginLoader>
#include <QStaticPlugin>
#include <QVector>

#include "device.h"
#include "kdeconnect_debug.h"
#include "kdeconnectconfig.h"
#include "kdeconnectplugin.h"

PluginLoader *PluginLoader::instance()
{
    static PluginLoader *instance = new PluginLoader();
    return instance;
}

PluginLoader::PluginLoader()
{
    const QVector<KPluginMetaData> data = KPluginMetaData::findPlugins(QStringLiteral("kdeconnect"));
    for (const KPluginMetaData &data : data) {
        plugins[data.pluginId()] = data;
    }
}

QStringList PluginLoader::getPluginList() const
{
    return plugins.keys();
}

bool PluginLoader::doesPluginExist(const QString &name) const
{
    return plugins.contains(name);
}

KPluginMetaData PluginLoader::getPluginInfo(const QString &name) const
{
    return plugins.value(name);
}

KdeConnectPlugin *PluginLoader::instantiatePluginForDevice(const QString &pluginName, Device *device) const
{
    const KPluginMetaData data = plugins.value(pluginName);
    if (!data.isValid()) {
        qCDebug(KDECONNECT_CORE) << "Plugin unknown" << pluginName;
        return nullptr;
    }

    const QStringList outgoingInterfaces = data.value(QStringLiteral("X-KdeConnect-OutgoingPacketType"), QStringList());
    const QVariantList args{QVariant::fromValue<Device *>(device), pluginName, outgoingInterfaces, data.iconName()};

    if (auto result = KPluginFactory::instantiatePlugin<KdeConnectPlugin>(data, device, args)) {
        qCDebug(KDECONNECT_CORE) << "Loaded plugin:" << data.pluginId();
        return result.plugin;
    } else {
        qCDebug(KDECONNECT_CORE) << "Error loading plugin" << result.errorText;
        return nullptr;
    }
}

QStringList PluginLoader::incomingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData &data : plugins) {
        QStringList rawValues = data.value(QStringLiteral("X-KdeConnect-SupportedPacketType"), QStringList());
        ret += QSet<QString>(rawValues.begin(), rawValues.end());
    }
    return ret.values();
}

QStringList PluginLoader::outgoingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData &data : plugins) {
        QStringList rawValues = data.value(QStringLiteral("X-KdeConnect-OutgoingPacketType"), QStringList());
        ret += QSet<QString>(rawValues.begin(), rawValues.end());
    }
    return ret.values();
}

QSet<QString> PluginLoader::pluginsForCapabilities(const QSet<QString> &incoming, const QSet<QString> &outgoing) const
{
    QSet<QString> ret;

    QString myDeviceType = KdeConnectConfig::instance().deviceType().toString();

    for (const KPluginMetaData &data : plugins) {
        // Check if the plugin support this device type
        const QStringList supportedDeviceTypes = data.value(QStringLiteral("X-KdeConnect-SupportedDeviceTypes"), QStringList());
        if (!supportedDeviceTypes.isEmpty()) {
            if (!supportedDeviceTypes.contains(myDeviceType)) {
                qCDebug(KDECONNECT_CORE) << "Not loading plugin" << data.pluginId() << "because this device of type" << myDeviceType
                                         << "is not supported. Supports:" << supportedDeviceTypes.join(QStringLiteral(", "));
                continue;
            }
        }

        // Check if capbilites intersect with the remote device
        const QStringList pluginIncomingCapabilities = data.rawData().value(QStringLiteral("X-KdeConnect-SupportedPacketType")).toVariant().toStringList();
        const QStringList pluginOutgoingCapabilities = data.rawData().value(QStringLiteral("X-KdeConnect-OutgoingPacketType")).toVariant().toStringList();

        bool capabilitiesEmpty = (pluginIncomingCapabilities.isEmpty() && pluginOutgoingCapabilities.isEmpty());
        if (!capabilitiesEmpty) {
            bool capabilitiesIntersect = (outgoing.intersects(QSet(pluginIncomingCapabilities.begin(), pluginIncomingCapabilities.end()))
                                          || incoming.intersects(QSet(pluginOutgoingCapabilities.begin(), pluginOutgoingCapabilities.end())));

            if (!capabilitiesIntersect) {
                qCDebug(KDECONNECT_CORE) << "Not loading plugin" << data.pluginId() << "because device doesn't support it";
                continue;
            }
        }

        // If we get here, the plugin can be loaded
        ret += data.pluginId();
    }

    return ret;
}
