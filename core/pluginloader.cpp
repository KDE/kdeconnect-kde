/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pluginloader.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KPluginMetaData>
#include <QPluginLoader>
#include <QStaticPlugin>
#include <QVector>

#include "kdeconnectconfig.h"
#include "core_debug.h"
#include "device.h"
#include "kdeconnectplugin.h"

// In older Qt released, qAsConst isnt available
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
    const QVector<KPluginMetaData> data = KPluginLoader::findPlugins(QStringLiteral("kdeconnect/"));
    for (const KPluginMetaData &metadata : data) {
        plugins[metadata.pluginId()] = metadata;
    }
#endif
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
    KdeConnectPlugin *ret = nullptr;

    KPluginMetaData service = plugins.value(pluginName);
    if (!service.isValid()) {
        qCDebug(KDECONNECT_CORE) << "Plugin unknown" << pluginName;
        return ret;
    }

#ifdef SAILFISHOS
    KPluginFactory *factory = pluginsFactories.value(pluginName);
#else
    KPluginLoader loader(service.fileName());
    KPluginFactory *factory = loader.factory();
    if (!factory) {
        qCDebug(KDECONNECT_CORE) << "KPluginFactory could not load the plugin:" << service.pluginId() << loader.errorString();
        return ret;
    }
#endif

    const QStringList outgoingInterfaces = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType"));

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

    QString myDeviceType = KdeConnectConfig::instance().deviceType().toString();

    for (const KPluginMetaData &service : qAsConst(plugins)) {

        // Check if the plugin support this device type
        const QSet<QString> supportedDeviceTypes = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedDeviceTypes")).toSet();
        if (!supportedDeviceTypes.isEmpty()) {
            if (!supportedDeviceTypes.contains(myDeviceType)) {
                qCDebug(KDECONNECT_CORE) << "Not loading plugin" << service.pluginId() << "because this device of type" << myDeviceType << "is not supported. Supports:" << supportedDeviceTypes.toList().join(QStringLiteral(", "));
                continue;
            }
        }

        // Check if capbilites intersect with the remote device
        const QSet<QString> pluginIncomingCapabilities =
            KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPacketType")).toSet();
        const QSet<QString> pluginOutgoingCapabilities =
            KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType")).toSet();

        bool capabilitiesEmpty = (pluginIncomingCapabilities.isEmpty() && pluginOutgoingCapabilities.isEmpty());
        if (!capabilitiesEmpty) {
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
            bool capabilitiesIntersect = (outgoing.intersects(pluginIncomingCapabilities) || incoming.intersects(pluginOutgoingCapabilities));
    #else
            QSet<QString> commonIncoming = incoming;
            commonIncoming.intersect(pluginOutgoingCapabilities);
            QSet<QString> commonOutgoing = outgoing;
            commonOutgoing.intersect(pluginIncomingCapabilities);
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
