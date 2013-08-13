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

#include "kdeconnectplugin.h"

#include <KServiceTypeTrader>
#include <KDebug>

#include "../device.h"

PluginLoader* PluginLoader::instance()
{
    static PluginLoader* instance = new PluginLoader();
    return instance;
}

PluginLoader::PluginLoader()
{
    KService::List offers = KServiceTypeTrader::self()->query("KdeConnect/Plugin");
    for(KService::List::const_iterator iter = offers.begin(); iter < offers.end(); ++iter) {
        KService::Ptr service = *iter;
        plugins[service->library()] = service;
    }
}

QStringList PluginLoader::getPluginList()
{
    return plugins.keys();
}

KPluginInfo PluginLoader::getPluginInfo(const QString& name) {

    KService::Ptr service = plugins[name];
    if (!service) {
        qDebug() << "Plugin unknown" << name;
        return KPluginInfo();
    }

    return KPluginInfo(service);
}

KdeConnectPlugin* PluginLoader::instantiatePluginForDevice(const QString& name, Device* device) {

    KService::Ptr service = plugins[name];
    if (!service) {
        qDebug() << "Plugin unknown" << name;
        return NULL;
    }

    KPluginFactory *factory = KPluginLoader(service->library()).factory();
    if (!factory) {
        qDebug() << "KPluginFactory could not load the plugin:" << service->library();
        return NULL;
    }

    QVariant deviceVariant;
    deviceVariant.setValue<Device*>(device);

    //FIXME: create<KdeConnectPlugin> return NULL
    QObject *plugin = factory->create<QObject>(device, QVariantList() << deviceVariant);
    if (!plugin) {
        qDebug() << "Error loading plugin";
        return NULL;
    }

    qDebug() << "Loaded plugin:" << service->name();
    return (KdeConnectPlugin*)plugin;
}

