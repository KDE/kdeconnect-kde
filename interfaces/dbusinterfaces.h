/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DBUSINTERFACES_H
#define DBUSINTERFACES_H

#include "kdeconnectinterfaces_export.h"

#include "generated/batteryinterface.h"
#include "generated/bigscreeninterface.h"
#include "generated/connectivityinterface.h"
#include "generated/conversationsinterface.h"
#include "generated/daemoninterface.h"
#include "generated/deviceclipboardinterface.h"
#include "generated/devicefindmyphoneinterface.h"
#include "generated/deviceinterface.h"
#include "generated/devicenotificationsinterface.h"
#include "generated/devicesftpinterface.h"
#include "generated/lockdeviceinterface.h"
#include "generated/mprisremoteinterface.h"
#include "generated/notificationinterface.h"
#include "generated/remotecommandsinterface.h"
#include "generated/remotecontrolinterface.h"
#include "generated/remotekeyboardinterface.h"
#include "generated/remotesystemvolumeinterface.h"
#include "generated/shareinterface.h"
#include "generated/smsinterface.h"
#include "generated/virtualmonitorinterface.h"

/**
 * Using these "proxy" classes just in case we need to rename the
 * interface, so we can change the class name in a single place.
 */
class KDECONNECTINTERFACES_EXPORT DaemonDbusInterface : public OrgKdeKdeconnectDaemonInterface
{
    Q_OBJECT
    Q_PROPERTY(QStringList customDevices READ customDevices WRITE setCustomDevices NOTIFY customDevicesChangedProxy)
public:
    explicit DaemonDbusInterface(QObject *parent = nullptr);

    static QString activatedService();

Q_SIGNALS:
    void customDevicesChangedProxy();
};

class KDECONNECTINTERFACES_EXPORT DeviceDbusInterface : public OrgKdeKdeconnectDeviceInterface
{
    Q_OBJECT
    // Workaround because qdbusxml2cpp is not generating NOTIFY for properties
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY reachableChangedProxy)
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY pairStateChangedProxy)
    Q_PROPERTY(bool isPairRequested READ isPairRequested NOTIFY pairStateChangedProxy)
    Q_PROPERTY(bool isPairRequestedByPeer READ isPairRequestedByPeer NOTIFY pairStateChangedProxy)
    Q_PROPERTY(int pairState READ pairState NOTIFY pairStateChangedProxy)
    Q_PROPERTY(QString name READ name NOTIFY nameChangedProxy)
    Q_PROPERTY(QString verificationKey READ verificationKey NOTIFY pairStateChangedProxy)

public:
    explicit DeviceDbusInterface(const QString &deviceId, QObject *parent = nullptr);

    Q_SCRIPTABLE QString id() const;
    Q_SCRIPTABLE void pluginCall(const QString &plugin, const QString &method);

Q_SIGNALS:
    void nameChangedProxy(const QString &name);
    void pairStateChangedProxy(int pairState);
    void reachableChangedProxy(bool reachable);

private:
    const QString m_id;
};

class KDECONNECTINTERFACES_EXPORT BatteryDbusInterface : public OrgKdeKdeconnectDeviceBatteryInterface
{
    Q_OBJECT
    Q_PROPERTY(int charge READ charge NOTIFY refreshedProxy)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY refreshedProxy)
public:
    explicit BatteryDbusInterface(const QString &deviceId, QObject *parent = nullptr);

Q_SIGNALS:
    void refreshedProxy(bool isCharging, int charge);
};

class KDECONNECTINTERFACES_EXPORT ConnectivityReportDbusInterface : public OrgKdeKdeconnectDeviceConnectivity_reportInterface
{
    Q_OBJECT
    Q_PROPERTY(QString cellularNetworkType READ cellularNetworkType NOTIFY refreshedProxy)
    Q_PROPERTY(int cellularNetworkStrength READ cellularNetworkStrength NOTIFY refreshedProxy)
public:
    explicit ConnectivityReportDbusInterface(const QString &deviceId, QObject *parent = nullptr);

Q_SIGNALS:
    void refreshedProxy(QString cellularNetworkType, int cellularNetworkStrength);
};

class KDECONNECTINTERFACES_EXPORT DeviceNotificationsDbusInterface : public OrgKdeKdeconnectDeviceNotificationsInterface
{
    Q_OBJECT
public:
    explicit DeviceNotificationsDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT NotificationDbusInterface : public OrgKdeKdeconnectDeviceNotificationsNotificationInterface
{
    Q_OBJECT
public:
    NotificationDbusInterface(const QString &deviceId, const QString &notificationId, QObject *parent = nullptr);

    QString notificationId()
    {
        return id;
    }

private:
    const QString id;
};

class KDECONNECTINTERFACES_EXPORT DeviceConversationsDbusInterface : public OrgKdeKdeconnectDeviceConversationsInterface
{
    Q_OBJECT
public:
    explicit DeviceConversationsDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT SftpDbusInterface : public OrgKdeKdeconnectDeviceSftpInterface
{
    Q_OBJECT
public:
    explicit SftpDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT MprisDbusInterface : public OrgKdeKdeconnectDeviceMprisremoteInterface
{
    Q_OBJECT
    // Workaround because qdbusxml2cpp is not generating NOTIFY for properties
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int length READ length NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString title READ title NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString artist READ artist NOTIFY propertiesChangedProxy)
    Q_PROPERTY(QString album READ album NOTIFY propertiesChangedProxy)

    Q_PROPERTY(QStringList playerList READ playerList NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY propertiesChangedProxy)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY propertiesChangedProxy)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY propertiesChangedProxy)
public:
    explicit MprisDbusInterface(const QString &deviceId, QObject *parent = nullptr);

Q_SIGNALS:
    void propertiesChangedProxy();
};

class KDECONNECTINTERFACES_EXPORT RemoteControlDbusInterface : public OrgKdeKdeconnectDeviceRemotecontrolInterface
{
    Q_OBJECT
public:
    explicit RemoteControlDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT LockDeviceDbusInterface : public OrgKdeKdeconnectDeviceLockdeviceInterface
{
    Q_OBJECT
    Q_PROPERTY(bool isLocked READ isLocked WRITE setIsLocked NOTIFY lockedChangedProxy)
public:
    explicit LockDeviceDbusInterface(const QString &deviceId, QObject *parent = nullptr);

Q_SIGNALS:
    void lockedChangedProxy(bool isLocked);
};

class KDECONNECTINTERFACES_EXPORT FindMyPhoneDeviceDbusInterface : public OrgKdeKdeconnectDeviceFindmyphoneInterface
{
    Q_OBJECT
public:
    explicit FindMyPhoneDeviceDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT RemoteCommandsDbusInterface : public OrgKdeKdeconnectDeviceRemotecommandsInterface
{
    Q_OBJECT
    // Workaround because qdbusxml2cpp is not generating CONSTANT for properties and qml complains at runtime
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY(bool canAddCommand READ canAddCommand CONSTANT)
public:
    explicit RemoteCommandsDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT RemoteKeyboardDbusInterface : public OrgKdeKdeconnectDeviceRemotekeyboardInterface
{
    Q_OBJECT
    Q_PROPERTY(bool remoteState READ remoteState NOTIFY remoteStateChangedProxy)
public:
    explicit RemoteKeyboardDbusInterface(const QString &deviceId, QObject *parent = nullptr);
Q_SIGNALS:
    void remoteStateChangedProxy(bool state);
};

class KDECONNECTINTERFACES_EXPORT SmsDbusInterface : public OrgKdeKdeconnectDeviceSmsInterface
{
    Q_OBJECT
public:
    explicit SmsDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT ShareDbusInterface : public OrgKdeKdeconnectDeviceShareInterface
{
    Q_OBJECT
public:
    explicit ShareDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT RemoteSystemVolumeDbusInterface : public OrgKdeKdeconnectDeviceRemotesystemvolumeInterface
{
    Q_OBJECT
    // Workaround because qdbusxml2cpp is not generating CONSTANT for properties and qml complains at runtime
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)
public:
    explicit RemoteSystemVolumeDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT BigscreenDbusInterface : public OrgKdeKdeconnectDeviceBigscreenInterface
{
    Q_OBJECT
public:
    explicit BigscreenDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT VirtualmonitorDbusInterface : public OrgKdeKdeconnectDeviceVirtualmonitorInterface
{
    Q_OBJECT
public:
    explicit VirtualmonitorDbusInterface(const QString &deviceId, QObject *parent = nullptr);
};

class KDECONNECTINTERFACES_EXPORT ClipboardDbusInterface : public OrgKdeKdeconnectDeviceClipboardInterface
{
    Q_OBJECT
    Q_PROPERTY(bool isAutoShareDisabled READ isAutoShareDisabled NOTIFY autoShareDisabledChangedProxy)
public:
    explicit ClipboardDbusInterface(const QString &deviceId, QObject *parent = nullptr);
Q_SIGNALS:
    void autoShareDisabledChangedProxy(bool b);
};

#endif
