/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pluginloader.h"

#include <QVector>
#include <QPluginLoader>
#include <KPluginMetaData>
#include <KPluginLoader>
#include <KPluginFactory>
#include <QStaticPlugin>

#include "core_debug.h"
#include "device.h"
#include "kdeconnectplugin.h"

//In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

PluginLoader* PluginLoader::instance()
{
    static PluginLoader* instance = new PluginLoader();
    return instance;
}

PluginLoader::PluginLoader()
{
#ifdef SAILFISHOS
    const QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
    for (auto& staticPlugin : staticPlugins) {
        QJsonObject jsonMetadata = staticPlugin.metaData().value(QStringLiteral("MetaData")).toObject();
        KPluginMetaData metadata(jsonMetadata, QString());
        if (metadata.serviceTypes().contains(QStringLiteral("KdeConnect/Plugin"))) {
            plugins.insert(metadata.pluginId(), metadata);
            pluginsFactories.insert(
                metadata.pluginId(),
                qobject_cast<KPluginFactory*>(staticPlugin.instance()));
        }
    }
#else
    const QVector<KPluginMetaData> data = KPluginLoader::findPlugins(QStringLiteral("kdeconnect/"));
    for (const KPluginMetaData& metadata : data) {
        plugins[metadata.pluginId()] = metadata;
    }
#endif
}

QStringList PluginLoader::getPluginList() const
{
    return plugins.keys();
}

KPluginMetaData PluginLoader::getPluginInfo(const QString& name) const
{
    return plugins.value(name);
}

KdeConnectPlugin* PluginLoader::instantiatePluginForDevice(const QString& pluginName, Device* device) const
{
    KPluginMetaData service = plugins.value(pluginName);

    const QStringList outgoingInterfaces = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType"));
    const QVariantList args = {QVariant::fromValue(device), pluginName, outgoingInterfaces, service.iconName()};

#ifdef SAILFISHOS
    KPluginFactory* factory = pluginsFactories.value(pluginName);

    const auto ret = factory->create<KdeConnectPlugin>(device, args);

    if (!ret) {
        qCDebug(KDECONNECT_CORE) << "Error loading plugin";
    }
    return ret;

#else
    auto result = KPluginLoader::instantiatePlugin<KdeConnectPlugin>(service.fileName(), device, args);

    if (result) {
        return *result;
    }

    qCDebug(KDECONNECT_CORE) << "Error loading plugin" << result.error();
    return nullptr;
#endif
}

QStringList PluginLoader::incomingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData& service : qAsConst(plugins)) {
        ret += KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPacketType")).toSet();
    }
    return ret.values();
}

QStringList PluginLoader::outgoingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData& service : qAsConst(plugins)) {
        ret += KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType")).toSet();
    }
    return ret.values();
}

QSet<QString> PluginLoader::pluginsForCapabilities(const QSet<QString>& incoming, const QSet<QString>& outgoing)
{
    QSet<QString> ret;

    for (const KPluginMetaData& service : qAsConst(plugins)) {
        const QSet<QString> pluginIncomingCapabilities = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPacketType")).toSet();
        const QSet<QString> pluginOutgoingCapabilities = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType")).toSet();

        bool capabilitiesEmpty = (pluginIncomingCapabilities.isEmpty() && pluginOutgoingCapabilities.isEmpty());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        bool capabilitiesIntersect = (outgoing.intersects(pluginIncomingCapabilities) || incoming.intersects(pluginOutgoingCapabilities));
#else
        QSet<QString> commonIncoming = incoming;
        commonIncoming.intersect(pluginOutgoingCapabilities);
        QSet<QString> commonOutgoing = outgoing;
        commonOutgoing.intersect(pluginIncomingCapabilities);
        bool capabilitiesIntersect = (!commonIncoming.isEmpty() || !commonOutgoing.isEmpty());
#endif

        if (capabilitiesIntersect || capabilitiesEmpty) {
            ret += service.pluginId();
        } else {
            qCDebug(KDECONNECT_CORE) << "Not loading plugin" << service.pluginId() <<  "because device doesn't support it";
        }
    }

    return ret;
}
