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
#include "interfaces/devicefindmyphoneinterface.h"
#include "interfaces/devicenotificationsinterface.h"
#include "interfaces/notificationinterface.h"
#include "interfaces/mprisremoteinterface.h"
#include "interfaces/remotecontrolinterface.h"
#include "interfaces/lockdeviceinterface.h"

/**
 * Using these "proxy" classes just in case we need to rename the
 * interface, so we can change the class name in a single place.
 */
class KDECONNECTINTERFACES_EXPORT DaemonDbusInterface
    : public OrgKdeKdeconnectDaemonInterface
{
    Q_OBJECT
public:
    explicit DaemonDbusInterface(QObject* parent = nullptr);
    virtual ~DaemonDbusInterface();

    static QString activatedService();

Q_SIGNALS:
    void deviceAdded(const QString &id);
};

class KDECONNECTINTERFACES_EXPORT DeviceDbusInterface
    : public OrgKdeKdeconnectDeviceInterface
{
    Q_OBJECT
//  TODO: Workaround because OrgKdeKdeconnectDeviceInterface is not generating
//  the signals for the properties
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY pairingChangedProxy)

public:
    explicit DeviceDbusInterface(const QString& deviceId, QObject* parent = nullptr);
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
    explicit DeviceBatteryDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    virtual ~DeviceBatteryDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT DeviceNotificationsDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsInterface
{
    Q_OBJECT
public:
    explicit DeviceNotificationsDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    virtual ~DeviceNotificationsDbusInterface();
};

class KDECONNECTINTERFACES_EXPORT NotificationDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsNotificationInterface
{
    Q_OBJECT
public:
    NotificationDbusInterface(const QString& deviceId, const QString& notificationId, QObject* parent = nullptr);
    virtual ~NotificationDbusInterface();
};


class KDECONNECTINTERFACES_EXPORT SftpDbusInterface
    : public OrgKdeKdeconnectDeviceSftpInterface
{
    Q_OBJECT
public:
    explicit SftpDbusInterface(const QString& deviceId, QObject* parent = nullptr);
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
    explicit MprisDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    virtual ~MprisDbusInterface();

Q_SIGNALS:
    void propertiesChangedProxy();
};

class KDECONNECTINTERFACES_EXPORT RemoteControlDbusInterface
    : public OrgKdeKdeconnectDeviceRemotecontrolInterface
{
    Q_OBJECT
public:
    explicit RemoteControlDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~RemoteControlDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT LockDeviceDbusInterface
    : public OrgKdeKdeconnectDeviceLockdeviceInterface
{
    Q_OBJECT
    Q_PROPERTY(bool isLocked READ isLocked WRITE setIsLocked NOTIFY lockedChangedProxy)
public:
    explicit LockDeviceDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    virtual ~LockDeviceDbusInterface();

Q_SIGNALS:
    void lockedChangedProxy(bool isLocked);
};

class KDECONNECTINTERFACES_EXPORT FindMyPhoneDeviceDbusInterface
    : public OrgKdeKdeconnectDeviceFindmyphoneInterface
{
    Q_OBJECT
public:
    explicit FindMyPhoneDeviceDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    virtual ~FindMyPhoneDeviceDbusInterface();
};

#endif
