/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectpluginkcm.h"

KdeConnectPluginKcm::KdeConnectPluginKcm(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
#if QT_VERSION_MAJOR < 6
    : KCModule(qobject_cast<QWidget *>(parent), args)
#else
    : KCModule(parent)
#endif
    , m_deviceId(args.at(0).toString())
    // The plugin name is the KCMs ID with the postfix removed
    , m_config(new KdeConnectPluginConfig(m_deviceId, data.pluginId().remove(QLatin1String("_config")), this))
{
    Q_ASSERT(data.isValid()); // Even if we have empty metadata, it should be valid!
}

#include "moc_kdeconnectpluginkcm.cpp"
