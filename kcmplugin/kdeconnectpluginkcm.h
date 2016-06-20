/**
 * Copyright 2015 Albert Vaca <albertvaka@gmail.com>
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

#ifndef KDECONNECTPLUGINKCM_H
#define KDECONNECTPLUGINKCM_H

#include <KCModule>

#include "core/kdeconnectpluginconfig.h"
#include "kdeconnectpluginkcm_export.h"

struct KdeConnectPluginKcmPrivate;

/**
 * Inheriting your plugin's KCM from this class gets you a easy way to share
 * configuration values between the KCM and the plugin.
 */
class KDECONNECTPLUGINKCM_EXPORT KdeConnectPluginKcm
    : public KCModule
{
    Q_OBJECT

public:
    KdeConnectPluginKcm(QWidget* parent, const QVariantList& args, const QString& componentName);
    ~KdeConnectPluginKcm() override;

    /**
     * The device this kcm is instantiated for
     */
    QString deviceId() const;

    /**
     * The object where to save the config, so the plugin can access it
     */
    KdeConnectPluginConfig* config() const;

private:
    QScopedPointer<KdeConnectPluginKcmPrivate> d;
};

#endif
