/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectpluginkcm.h"

struct KdeConnectPluginKcmPrivate {
    QString m_deviceId;
    QString m_pluginName;
    KdeConnectPluginConfig *m_config = nullptr;
};

KdeConnectPluginKcm::KdeConnectPluginKcm(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
#if QT_VERSION_MAJOR < 6
    : KCModule(qobject_cast<QWidget *>(parent), args)
#else
    : KCModule(parent)
#endif
    , d(new KdeConnectPluginKcmPrivate())
{
    Q_ASSERT(data.isValid()); // Even if we have empty metadata, it should be valid!
    d->m_deviceId = args.at(0).toString();
    const static QRegularExpression removeConfigPostfix(QStringLiteral("_config$"));
    d->m_pluginName = data.pluginId().remove(removeConfigPostfix);

    // The parent of the config should be the plugin itself
    d->m_config = new KdeConnectPluginConfig(d->m_deviceId, d->m_pluginName);
}

KdeConnectPluginKcm::~KdeConnectPluginKcm()
{
    delete d->m_config;
}

KdeConnectPluginConfig *KdeConnectPluginKcm::config() const
{
    return d->m_config;
}

QString KdeConnectPluginKcm::deviceId() const
{
    return d->m_deviceId;
}

#include "moc_kdeconnectpluginkcm.cpp"
