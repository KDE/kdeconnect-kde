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

#ifndef KDECONNECTCONFIG_H
#define KDECONNECTCONFIG_H

#include <QDir>

#include "kdeconnectcore_export.h"

class QSslCertificate;
namespace QCA {
    class PrivateKey;
    class PublicKey;
}

class KDECONNECTCORE_EXPORT KdeConnectConfig
{
public:
    struct DeviceInfo {
        QString deviceName;
        QString deviceType;
    };

    static KdeConnectConfig* instance();

    /*
     * Our own info
     */

    QString deviceId();
    QString name();
    QString deviceType();

    QString privateKeyPath();
    QCA::PrivateKey privateKey();
    QCA::PublicKey publicKey();

    QString certificatePath();
    QSslCertificate certificate();

    void setName(const QString& name);

    /*
     * Trusted devices
     */

    QStringList trustedDevices(); //list of ids
    void removeTrustedDevice(const QString& id);
    void addTrustedDevice(const QString& id, const QString& name, const QString& type);
    KdeConnectConfig::DeviceInfo getTrustedDevice(const QString& id);

    void setDeviceProperty(const QString& deviceId, const QString& name, const QString& value);
    QString getDeviceProperty(const QString& deviceId, const QString& name, const QString& defaultValue = QString());

    /*
     * Paths for config files, there is no guarantee the directories already exist
     */
    QDir baseConfigDir();
    QDir deviceConfigDir(const QString& deviceId);
    QDir pluginConfigDir(const QString& deviceId, const QString& pluginName); //Used by KdeConnectPluginConfig

private:
    KdeConnectConfig();

private:

    struct KdeConnectConfigPrivate* d;
};

#endif
