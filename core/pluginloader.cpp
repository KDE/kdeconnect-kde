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

#include "core_debug.h"
#include "device.h"
#include "kdeconnectconfig.h"
#include "kdeconnectplugin.h"

// In older Qt released, qAsConst isn't available
#include "qtcompat_p.h"

PluginLoader *PluginLoader::instance()
{
    static PluginLoader *instance = new PluginLoader();
    return instance;
}

PluginLoader::PluginLoader()
{
#ifdef SAILFISHOS
    const QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (auto &staticPlugin : staticPlugins) {
        QJsonObject jsonMetadata = staticPlugin.metaData().value(QStringLiteral("MetaData")).toObject();
        KPluginMetaData metadata(jsonMetadata, QString());
        if (metadata.serviceTypes().contains(QStringLiteral("KdeConnect/Plugin"))) {
            plugins.insert(metadata.pluginId(), metadata);
            pluginsFactories.insert(metadata.pluginId(), qobject_cast<KPluginFactory *>(staticPlugin.instance()));
        }
    }
#else
    const QVector<KPluginMetaData> data = KPluginMetaData::findPlugins(QStringLiteral("kdeconnect/"));
    for (const KPluginMetaData &metadata : data) {
        plugins[metadata.pluginId()] = metadata;
    }
#endif
}

QStringList PluginLoader::getPluginList() const
{
    return plugins.keys();
}

KPluginMetaData PluginLoader::getPluginInfo(const QString &name) const
{
    return plugins.value(name);
}

KdeConnectPlugin *PluginLoader::instantiatePluginForDevice(const QString &pluginName, Device *device) const
{
    KdeConnectPlugin *ret = nullptr;

    KPluginMetaData service = plugins.value(pluginName);
    if (!service.isValid()) {
        qCDebug(KDECONNECT_CORE) << "Plugin unknown" << pluginName;
        return ret;
    }

#ifdef SAILFISHOS
    KPluginFactory *factory = pluginsFactories.value(pluginName);
#else
    auto factoryResult = KPluginFactory::loadFactory(service);
    if (!factoryResult) {
        qCDebug(KDECONNECT_CORE) << "KPluginFactory could not load the plugin:" << service.pluginId() << factoryResult.errorString;
        return ret;
    }
    KPluginFactory *factory = factoryResult.plugin;
#endif
    const QStringList outgoingInterfaces = service.rawData().value(QStringLiteral("X-KdeConnect-OutgoingPacketType")).toVariant().toStringList();

    QVariant deviceVariant = QVariant::fromValue<Device *>(device);

    ret = factory->create<KdeConnectPlugin>(device, QVariantList() << deviceVariant << pluginName << outgoingInterfaces << service.iconName());
    if (!ret) {
        qCDebug(KDECONNECT_CORE) << "Error loading plugin";
        return ret;
    }

    // qCDebug(KDECONNECT_CORE) << "Loaded plugin:" << service.pluginId();
    return ret;
}

QStringList PluginLoader::incomingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData &service : qAsConst(plugins)) {
        QStringList rawValues = service.value(QStringLiteral("X-KdeConnect-SupportedPacketType"), QStringList());
        ret += QSet<QString>(rawValues.begin(), rawValues.end());
    }
    return ret.values();
}

QStringList PluginLoader::outgoingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData &service : qAsConst(plugins)) {
        QStringList rawValues = service.value(QStringLiteral("X-KdeConnect-OutgoingPacketType"), QStringList());
        ret += QSet<QString>(rawValues.begin(), rawValues.end());
    }
    return ret.values();
}

QSet<QString> PluginLoader::pluginsForCapabilities(const QSet<QString> &incoming, const QSet<QString> &outgoing)
{
    QSet<QString> ret;

    QString myDeviceType = KdeConnectConfig::instance().deviceType();

    for (const KPluginMetaData &service : qAsConst(plugins)) {
        // Check if the plugin support this device type
        const QStringList supportedDeviceTypes = service.rawData().value(QStringLiteral("X-KdeConnect-SupportedDeviceTypes")).toVariant().toStringList();
        if (!supportedDeviceTypes.isEmpty()) {
            if (!supportedDeviceTypes.contains(myDeviceType)) {
                qCDebug(KDECONNECT_CORE) << "Not loading plugin" << service.pluginId() << "because this device of type" << myDeviceType
                                         << "is not supported. Supports:" << supportedDeviceTypes.toList().join(QStringLiteral(", "));
                continue;
            }
        }

        // Check if capbilites intersect with the remote device
        const QStringList pluginIncomingCapabilities = service.rawData().value(QStringLiteral("X-KdeConnect-SupportedPacketType")).toVariant().toStringList();
        const QStringList pluginOutgoingCapabilities = service.rawData().value(QStringLiteral("X-KdeConnect-OutgoingPacketType")).toVariant().toStringList();

        bool capabilitiesEmpty = (pluginIncomingCapabilities.isEmpty() && pluginOutgoingCapabilities.isEmpty());
        if (!capabilitiesEmpty) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
            bool capabilitiesIntersect = (outgoing.intersects(QSet(pluginIncomingCapabilities.begin(), pluginIncomingCapabilities.end()))
                                          || incoming.intersects(QSet(pluginOutgoingCapabilities.begin(), pluginOutgoingCapabilities.end())));
#else
            QSet<QString> commonIncoming = incoming;
            commonIncoming.intersect(pluginOutgoingCapabilities.toSet());
            QSet<QString> commonOutgoing = outgoing;
            commonOutgoing.intersect(pluginIncomingCapabilities.toSet());
            bool capabilitiesIntersect = (!commonIncoming.isEmpty() || !commonOutgoing.isEmpty());
#endif

            if (!capabilitiesIntersect) {
                qCDebug(KDECONNECT_CORE) << "Not loading plugin" << service.pluginId() << "because device doesn't support it";
                continue;
            }
        }

        // If we get here, the plugin can be loaded
        ret += service.pluginId();
    }

    return ret;
}
