/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    : KCModule(parent, args)
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


