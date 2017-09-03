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

#include "kdeconnectpluginkcm.h"

#include <KAboutData>
#include <KService>

struct KdeConnectPluginKcmPrivate
{
    QString m_deviceId;
    QString m_pluginName;
    KdeConnectPluginConfig* m_config;
};

KdeConnectPluginKcm::KdeConnectPluginKcm(QWidget* parent, const QVariantList& args, const QString& componentName)
    : KCModule(KAboutData::pluginData(componentName), parent, args)
    , d(new KdeConnectPluginKcmPrivate())
{

    d->m_deviceId = args.at(0).toString();
    //The parent of the config should be the plugin itself
    d->m_pluginName = KService::serviceByDesktopName(componentName).constData()->property(QStringLiteral("X-KDE-ParentComponents")).toString();

    d->m_config = new KdeConnectPluginConfig(d->m_deviceId, d->m_pluginName);
}

KdeConnectPluginKcm::~KdeConnectPluginKcm()
{
    delete d->m_config;
}

KdeConnectPluginConfig* KdeConnectPluginKcm::config() const
{
    return d->m_config;
}

QString KdeConnectPluginKcm::deviceId() const
{
    return d->m_deviceId;
}


