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

#include <KServiceTypeTrader>

#include "kdebugnamespace.h"
#include "device.h"
#include "kdeconnectplugin.h"

PluginLoader* PluginLoader::instance()
{
    static PluginLoader* instance = new PluginLoader();
    return instance;
}

PluginLoader::PluginLoader()
{
    KService::List offers = KServiceTypeTrader::self()->query("KdeConnect/Plugin");
    for(KService::List::const_iterator iter = offers.constBegin(); iter != offers.constEnd(); ++iter) {
        KService::Ptr service = *iter;
        plugins[service->library()] = service;
    }
}

QStringList PluginLoader::getPluginList() const
{
    return plugins.keys();
}

KPluginInfo PluginLoader::getPluginInfo(const QString& name) const
{
    KService::Ptr service = plugins[name];
    if (!service) {
        kDebug(debugArea()) << "Plugin unknown" << name;
        return KPluginInfo();
    }

    return KPluginInfo(service);
}

KdeConnectPlugin* PluginLoader::instantiatePluginForDevice(const QString& name, Device* device) const
{
    KdeConnectPlugin* ret = 0;

    KService::Ptr service = plugins[name];
    if (!service) {
        kDebug(debugArea()) << "Plugin unknown" << name;
        return ret;
    }

    KPluginFactory *factory = KPluginLoader(service->library()).factory();
    if (!factory) {
        kDebug(debugArea()) << "KPluginFactory could not load the plugin:" << service->library();
        return ret;
    }

    QStringList outgoingInterfaces = service->property("X-KdeConnect-OutgoingPackageType", QVariant::StringList).toStringList();

    QVariant deviceVariant = QVariant::fromValue<Device*>(device);

    ret = factory->create<KdeConnectPlugin>(device, QVariantList() << deviceVariant << outgoingInterfaces);
    if (!ret) {
        kDebug(debugArea()) << "Error loading plugin";
        return ret;
    }

    kDebug(debugArea()) << "Loaded plugin:" << service->name();
    return ret;
}

KService::Ptr PluginLoader::pluginService(const QString& pluginName) const
{
    return plugins[pluginName];
}

QStringList PluginLoader::incomingInterfaces() const
{
    QSet<QString> ret;
    foreach(const KService::Ptr& service, plugins) {
        ret += service->property("X-KdeConnect-SupportedPackageType", QVariant::StringList).toStringList().toSet();
    }
    return ret.toList();
}

QStringList PluginLoader::outgoingInterfaces() const
{
    QSet<QString> ret;
    foreach(const KService::Ptr& service, plugins) {
        ret += service->property("X-KdeConnect-OutgoingPackageType", QVariant::StringList).toStringList().toSet();
    }
    return ret.toList();
}
