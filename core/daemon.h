/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_DAEMON_H
#define KDECONNECT_DAEMON_H

#include <QMap>
#include <QObject>
#include <QSet>

#include "device.h"
#include "kdeconnectcore_export.h"

class NetworkPacket;
class DeviceLink;
class Device;
class QNetworkAccessManager;
class KJobTrackerInterface;

class KDECONNECTCORE_EXPORT Daemon : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
    Q_PROPERTY(QStringList pairingRequests READ pairingRequests NOTIFY pairingRequestsChanged)
    Q_PROPERTY(QStringList customDevices READ customDevices WRITE setCustomDevices NOTIFY customDevicesChanged)

public:
    explicit Daemon(QObject *parent, bool testMode = false);
    ~Daemon() override;

    static Daemon *instance();

    QList<Device *> devicesList() const;

    virtual void askPairingConfirmation(Device *device) = 0;
    virtual void reportError(const QString &title, const QString &description) = 0;
    virtual void quit() = 0;
    QNetworkAccessManager *networkAccessManager();
    virtual KJobTrackerInterface *jobTracker() = 0;

    Device *getDevice(const QString &deviceId);

    QSet<LinkProvider *> getLinkProviders() const;

    QStringList pairingRequests() const;

    // Custom devices
    QStringList customDevices() const;
    void setCustomDevices(const QStringList &addresses);

    Q_SCRIPTABLE QString selfId() const;
public Q_SLOTS:
    Q_SCRIPTABLE void forceOnNetworkChange();

    /// don't try to turn into Q_PROPERTY, it doesn't work
    Q_SCRIPTABLE QString announcedName();
    Q_SCRIPTABLE void setAnnouncedName(const QString &name);

    // Returns a list of ids. The respective devices can be manipulated using the dbus path: "/modules/kdeconnect/Devices/"+id
    Q_SCRIPTABLE QStringList devices(bool onlyReachable = false, bool onlyPaired = false) const;
    Q_SCRIPTABLE QMap<QString, QString> deviceNames(bool onlyReachable = false, bool onlyPaired = false) const;

    Q_SCRIPTABLE QString deviceIdByName(const QString &name) const;
    Q_SCRIPTABLE QVector<QStringList> linkProviders() const;
    Q_SCRIPTABLE virtual void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) = 0;

Q_SIGNALS:
    Q_SCRIPTABLE void deviceAdded(const QString &id);
    Q_SCRIPTABLE void deviceRemoved(const QString &id); // Note that paired devices will never be removed
    Q_SCRIPTABLE void deviceVisibilityChanged(const QString &id, bool isVisible);
    Q_SCRIPTABLE void deviceListChanged(); // Emitted when any of deviceAdded, deviceRemoved or deviceVisibilityChanged is emitted
    Q_SCRIPTABLE void announcedNameChanged(const QString &announcedName);
    Q_SCRIPTABLE void pairingRequestsChanged();
    Q_SCRIPTABLE void customDevicesChanged(const QStringList &customDevices);

private Q_SLOTS:
    void onNewDeviceLink(DeviceLink *dl);
    void onDeviceStatusChanged();

private:
    void init();

protected:
    void addDevice(Device *device);
    void removeDevice(Device *d);

    const std::unique_ptr<struct DaemonPrivate> d;
};

#endif
