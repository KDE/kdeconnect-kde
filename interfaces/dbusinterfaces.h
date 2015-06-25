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

#ifndef DBUSINTERFACES_H
#define DBUSINTERFACES_H

#include "interfaces/kdeconnectinterfaces_export.h"

#include "interfaces/daemoninterface.h"
#include "interfaces/deviceinterface.h"
#include "interfaces/devicebatteryinterface.h"
#include "interfaces/devicesftpinterface.h"
#include "interfaces/devicenotificationsinterface.h"
#include "interfaces/notificationinterface.h"
#include "interfaces/mprisremoteinterface.h"

/**
 * Using these "proxy" classes just in case we need to rename the
 * interface, so we can change the class name in a single place.
 */
class KDECONNECTINTERFACES_EXPORT DaemonDbusInterface
    : public OrgKdeKdeconnectDaemonInterface
{
    Q_OBJECT
public:
    DaemonDbusInterface(QObject* parent = 0);
    virtual ~DaemonDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT DeviceDbusInterface
    : public OrgKdeKdeconnectDeviceInterface
{
    Q_OBJECT
//  TODO: Workaround because OrgKdeKdeconnectDeviceInterface is not generating
//  the signals for the properties
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY pairingChangedProxy)

public:
    DeviceDbusInterface(const QString& deviceId, QObject* parent = 0);
    virtual ~DeviceDbusInterface();

    Q_SCRIPTABLE QString id() const;
    Q_SCRIPTABLE void pluginCall(const QString &plugin, const QString &method);

Q_SIGNALS:
    void pairingChangedProxy(bool paired);

private:
    const QString m_id;
};

class KDECONNECTINTERFACES_EXPORT DeviceBatteryDbusInterface
    : public OrgKdeKdeconnectDeviceBatteryInterface
{
    Q_OBJECT
public:
    DeviceBatteryDbusInterface(const QString& deviceId, QObject* parent = 0);
    virtual ~DeviceBatteryDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT DeviceNotificationsDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsInterface
{
    Q_OBJECT
public:
    DeviceNotificationsDbusInterface(const QString& deviceId, QObject* parent = 0);
    virtual ~DeviceNotificationsDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT NotificationDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsNotificationInterface
{
    Q_OBJECT
public:
    NotificationDbusInterface(const QString& deviceId, const QString& notificationId, QObject* parent = 0);
    virtual ~NotificationDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT SftpDbusInterface
    : public OrgKdeKdeconnectDeviceSftpInterface
{
    Q_OBJECT
public:
    SftpDbusInterface(const QString& deviceId, QObject* parent = 0);
    virtual ~SftpDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT MprisDbusInterface
    : public OrgKdeKdeconnectDeviceMprisremoteInterface
{
    Q_OBJECT
//  TODO: Workaround because qdbusxml2cpp is not generating
//  the signals for the properties
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int length READ length NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString nowPlaying READ nowPlaying NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QStringList playerList READ playerList NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY propertiesChangedProxy)
public:
    MprisDbusInterface(const QString& deviceId, QObject* parent = 0);
    virtual ~MprisDbusInterface();

Q_SIGNALS:
    void propertiesChangedProxy();
};

#endif
