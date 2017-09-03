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

#ifndef KDECONNECT_DAEMON_H
#define KDECONNECT_DAEMON_H

#include <QObject>
#include <QSet>
#include <QMap>

#include "kdeconnectcore_export.h"
#include "device.h"

class NetworkPackage;
class DeviceLink;
class Device;
class QNetworkAccessManager;

class KDECONNECTCORE_EXPORT Daemon
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
    Q_PROPERTY(bool isDiscoveringDevices READ isDiscoveringDevices)
    Q_PROPERTY(QStringList pairingRequests READ pairingRequests NOTIFY pairingRequestsChanged)

public:
    explicit Daemon(QObject* parent, bool testMode = false);
    ~Daemon() override;

    static Daemon* instance();

    QList<Device*> devicesList() const;

    virtual void askPairingConfirmation(Device* device) = 0;
    virtual void reportError(const QString& title, const QString& description) = 0;
    virtual QNetworkAccessManager* networkAccessManager();

    Device* getDevice(const QString& deviceId);

    QStringList pairingRequests() const;

    Q_SCRIPTABLE QString selfId() const;
public Q_SLOTS:
    Q_SCRIPTABLE void acquireDiscoveryMode(const QString& id);
    Q_SCRIPTABLE void releaseDiscoveryMode(const QString& id);

    Q_SCRIPTABLE void forceOnNetworkChange();

    ///don't try to turn into Q_PROPERTY, it doesn't work
    Q_SCRIPTABLE QString announcedName();
    Q_SCRIPTABLE void setAnnouncedName(const QString& name);

    //Returns a list of ids. The respective devices can be manipulated using the dbus path: "/modules/kdeconnect/Devices/"+id
    Q_SCRIPTABLE QStringList devices(bool onlyReachable = false, bool onlyPaired = false) const;

    Q_SCRIPTABLE QString deviceIdByName(const QString& name) const;

Q_SIGNALS:
    Q_SCRIPTABLE void deviceAdded(const QString& id);
    Q_SCRIPTABLE void deviceRemoved(const QString& id); //Note that paired devices will never be removed
    Q_SCRIPTABLE void deviceVisibilityChanged(const QString& id, bool isVisible);
    Q_SCRIPTABLE void announcedNameChanged(const QString& announcedName);
    Q_SCRIPTABLE void pairingRequestsChanged();

private Q_SLOTS:
    void onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl);
    void onDeviceStatusChanged();

private:
    void addDevice(Device* device);
    bool isDiscoveringDevices() const;
    void removeDevice(Device* d);
    void cleanDevices();

    QScopedPointer<struct DaemonPrivate> d;
};

#endif
