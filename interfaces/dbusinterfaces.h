/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DBUSINTERFACES_H
#define DBUSINTERFACES_H

#include "kdeconnectinterfaces_export.h"

#include "daemoninterface.h"
#include "deviceinterface.h"
#include "batteryinterface.h"
#include "connectivityinterface.h"
#include "devicesftpinterface.h"
#include "devicefindmyphoneinterface.h"
#include "devicenotificationsinterface.h"
#include "notificationinterface.h"
#include "mprisremoteinterface.h"
#include "remotecontrolinterface.h"
#include "lockdeviceinterface.h"
#include "remotecommandsinterface.h"
#include "remotekeyboardinterface.h"
#include "smsinterface.h"
#include "conversationsinterface.h"
#include "shareinterface.h"
#include "remotesystemvolumeinterface.h"
#include "bigscreeninterface.h"
#include "virtualmonitorinterface.h"

/**
 * Using these "proxy" classes just in case we need to rename the
 * interface, so we can change the class name in a single place.
 */
class KDECONNECTINTERFACES_EXPORT DaemonDbusInterface
    : public OrgKdeKdeconnectDaemonInterface
{
    Q_OBJECT
    Q_PROPERTY(QStringList customDevices
               READ customDevices WRITE setCustomDevices NOTIFY customDevicesChangedProxy)
public:
    explicit DaemonDbusInterface(QObject* parent = nullptr);
    ~DaemonDbusInterface() override;

    static QString activatedService();

Q_SIGNALS:
    void deviceAdded(const QString& id);
    void pairingRequestsChangedProxy();
    void customDevicesChangedProxy();
};

class KDECONNECTINTERFACES_EXPORT DeviceDbusInterface
    : public OrgKdeKdeconnectDeviceInterface
{
    Q_OBJECT
//  TODO: Workaround because OrgKdeKdeconnectDeviceInterface is not generating
//  the signals for the properties
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY reachableChangedProxy)
    Q_PROPERTY(bool isTrusted READ isTrusted NOTIFY trustedChangedProxy)
    Q_PROPERTY(QString name READ name NOTIFY nameChangedProxy)
    Q_PROPERTY(bool hasPairingRequests READ hasPairingRequests NOTIFY hasPairingRequestsChangedProxy)

public:
    explicit DeviceDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~DeviceDbusInterface() override;

    Q_SCRIPTABLE QString id() const;
    Q_SCRIPTABLE void pluginCall(const QString& plugin, const QString& method);

Q_SIGNALS:
    void nameChangedProxy(const QString& name);
    void trustedChangedProxy(bool paired);
    void reachableChangedProxy(bool reachable);
    void hasPairingRequestsChangedProxy(bool);

private:
    const QString m_id;
};

class KDECONNECTINTERFACES_EXPORT BatteryDbusInterface
    : public OrgKdeKdeconnectDeviceBatteryInterface
{
    Q_OBJECT
    Q_PROPERTY(int charge READ charge NOTIFY refreshedProxy)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY refreshedProxy)
public:
    explicit BatteryDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~BatteryDbusInterface() override;

Q_SIGNALS:
    void refreshedProxy(bool isCharging, int charge);
};

class KDECONNECTINTERFACES_EXPORT ConnectivityReportDbusInterface
    : public OrgKdeKdeconnectDeviceConnectivity_reportInterface
{
    Q_OBJECT
    Q_PROPERTY(QString cellularNetworkType READ cellularNetworkType NOTIFY refreshedProxy)
    Q_PROPERTY(int cellularNetworkStrength READ cellularNetworkStrength NOTIFY refreshedProxy)
public:
    explicit ConnectivityReportDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~ConnectivityReportDbusInterface() override;

Q_SIGNALS:
    void refreshedProxy(QString cellularNetworkType, int cellularNetworkStrength);
};

class KDECONNECTINTERFACES_EXPORT DeviceNotificationsDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsInterface
{
    Q_OBJECT
public:
    explicit DeviceNotificationsDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~DeviceNotificationsDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT NotificationDbusInterface
    : public OrgKdeKdeconnectDeviceNotificationsNotificationInterface
{
    Q_OBJECT
public:
    NotificationDbusInterface(const QString& deviceId, const QString& notificationId, QObject* parent = nullptr);
    ~NotificationDbusInterface() override;

    QString notificationId() { return id; }
private:
    const QString id;
};

class KDECONNECTINTERFACES_EXPORT DeviceConversationsDbusInterface
    : public OrgKdeKdeconnectDeviceConversationsInterface
{
    Q_OBJECT
public:
    explicit DeviceConversationsDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~DeviceConversationsDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT SftpDbusInterface
    : public OrgKdeKdeconnectDeviceSftpInterface
{
    Q_OBJECT
public:
    explicit SftpDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~SftpDbusInterface() override;
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
    Q_PROPERTY(QString title READ title NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString artist READ artist NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString album READ album NOTIFY propertiesChangedProxy)

    Q_PROPERTY(QStringList playerList READ playerList NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY propertiesChangedProxy)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY propertiesChangedProxy)
public:
    explicit MprisDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~MprisDbusInterface() override;

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
    ~LockDeviceDbusInterface() override;

Q_SIGNALS:
    void lockedChangedProxy(bool isLocked);
};

class KDECONNECTINTERFACES_EXPORT FindMyPhoneDeviceDbusInterface
    : public OrgKdeKdeconnectDeviceFindmyphoneInterface
{
    Q_OBJECT
public:
    explicit FindMyPhoneDeviceDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~FindMyPhoneDeviceDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT RemoteCommandsDbusInterface
    : public OrgKdeKdeconnectDeviceRemotecommandsInterface
{
    Q_OBJECT
public:
    explicit RemoteCommandsDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~RemoteCommandsDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT RemoteKeyboardDbusInterface
    : public OrgKdeKdeconnectDeviceRemotekeyboardInterface
{
    Q_OBJECT
    Q_PROPERTY(bool remoteState READ remoteState NOTIFY remoteStateChanged)
public:
    explicit RemoteKeyboardDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~RemoteKeyboardDbusInterface() override;
Q_SIGNALS:
    void remoteStateChanged(bool state);
};

class KDECONNECTINTERFACES_EXPORT SmsDbusInterface
    : public OrgKdeKdeconnectDeviceSmsInterface
{
    Q_OBJECT
public:
    explicit SmsDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~SmsDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT ShareDbusInterface
    : public OrgKdeKdeconnectDeviceShareInterface
{
    Q_OBJECT
public:
    explicit ShareDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~ShareDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT RemoteSystemVolumeDbusInterface
    : public OrgKdeKdeconnectDeviceRemotesystemvolumeInterface
{
    Q_OBJECT
public:
    explicit RemoteSystemVolumeDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~RemoteSystemVolumeDbusInterface() override = default;
};

class KDECONNECTINTERFACES_EXPORT BigscreenDbusInterface
    : public OrgKdeKdeconnectDeviceBigscreenInterface
{
    Q_OBJECT
public:
    explicit BigscreenDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~BigscreenDbusInterface() override;
};

class KDECONNECTINTERFACES_EXPORT VirtualmonitorDbusInterface
    : public OrgKdeKdeconnectDeviceVirtualmonitorInterface
{
    Q_OBJECT
public:
    explicit VirtualmonitorDbusInterface(const QString& deviceId, QObject* parent = nullptr);
    ~VirtualmonitorDbusInterface() override;
};

template <typename T, typename W>
static void setWhenAvailable(const QDBusPendingReply<T>& pending, W func, QObject* parent)
{
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pending, parent);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                    parent, [func](QDBusPendingCallWatcher* watcher) {
                        watcher->deleteLater();
                        QDBusPendingReply<T> reply = *watcher;
                        func(reply.value());
                    });
}

#endif
