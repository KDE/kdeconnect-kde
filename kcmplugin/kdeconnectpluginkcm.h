/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KCModule>
#include <KPluginMetaData> // Not in KCModule header of older KF5 versions
#include <kconfigwidgets_version.h>

#include "core/kdeconnectpluginconfig.h"
#include "kdeconnectpluginkcm_export.h"

/**
 * Inheriting your plugin's KCM from this class gets you a easy way to share
 * configuration values between the KCM and the plugin.
 */
class KDECONNECTPLUGINKCM_EXPORT KdeConnectPluginKcm : public KCModule
{
    Q_OBJECT

public:
    explicit KdeConnectPluginKcm(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    /**
     * The device this kcm is instantiated for
     */
    QString deviceId() const
    {
        return m_deviceId;
    }

    /**
     * The object where to save the config, so the plugin can access it
     */
    KdeConnectPluginConfig *config() const
    {
        return m_config;
    }

private:
    const QString m_deviceId;
    const QString m_pluginName;
    KdeConnectPluginConfig *const m_config;
};
