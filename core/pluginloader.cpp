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

#include "pluginloader.h"

#include <KPluginMetaData>
#include <KPluginLoader>
#include <KPluginFactory>

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
    const QVector<KPluginMetaData> data = KPluginLoader::findPlugins(QStringLiteral("kdeconnect/"));
    for (const KPluginMetaData& metadata : data) {
        plugins[metadata.pluginId()] = metadata;
    }
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
    KdeConnectPlugin* ret = Q_NULLPTR;

    KPluginMetaData service = plugins.value(pluginName);
    if (!service.isValid()) {
        qCDebug(KDECONNECT_CORE) << "Plugin unknown" << pluginName;
        return ret;
    }

    KPluginLoader loader(service.fileName());
    KPluginFactory* factory = loader.factory();
    if (!factory) {
        qCDebug(KDECONNECT_CORE) << "KPluginFactory could not load the plugin:" << service.pluginId() << loader.errorString();
        return ret;
    }

    const QStringList outgoingInterfaces = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType"));

    QVariant deviceVariant = QVariant::fromValue<Device*>(device);

    ret = factory->create<KdeConnectPlugin>(device, QVariantList() << deviceVariant << pluginName << outgoingInterfaces);
    if (!ret) {
        qCDebug(KDECONNECT_CORE) << "Error loading plugin";
        return ret;
    }

    //qCDebug(KDECONNECT_CORE) << "Loaded plugin:" << service.pluginId();
    return ret;
}

QStringList PluginLoader::incomingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData& service : qAsConst(plugins)) {
        ret += KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPacketType")).toSet();
    }
    return ret.toList();
}

QStringList PluginLoader::outgoingCapabilities() const
{
    QSet<QString> ret;
    for (const KPluginMetaData& service : qAsConst(plugins)) {
        ret += KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-OutgoingPacketType")).toSet();
    }
    return ret.toList();
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
