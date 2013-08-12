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

#include "packageinterface.h"

#include <KServiceTypeTrader>
#include <KDebug>

PluginLoader::PluginLoader(QObject * parent)
  : QObject(parent)
{
}

PluginLoader::~PluginLoader()
{
}

void PluginLoader::loadAllPlugins()
{
    kDebug() << "Load all plugins";
    KService::List offers = KServiceTypeTrader::self()->query("KdeConnect/Plugin");

    qDebug() << "LO TRAIGO DE OFERTA CHACHO" << offers;

    KService::List::const_iterator iter;
    for(iter = offers.begin(); iter < offers.end(); ++iter)
    {
       QString error;
       KService::Ptr service = *iter;

        KPluginFactory *factory = KPluginLoader(service->library()).factory();

        if (!factory)
        {
            //KMessageBox::error(0, i18n("<html><p>KPluginFactory could not load the plugin:<br /><i>%1</i></p></html>",
              //                         service->library()));
            kError(5001) << "KPluginFactory could not load the plugin:" << service->library();
            continue;
        }

       PackageInterface *plugin = factory->create<PackageInterface>(this);

       if (plugin) {
           kDebug() << "Load plugin:" << service->name();
           emit pluginLoaded(plugin);
       } else {
           kDebug() << error;
       }
    }
}