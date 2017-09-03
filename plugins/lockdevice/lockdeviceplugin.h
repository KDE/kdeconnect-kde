/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef LOCKDEVICEPLUGIN_H
#define LOCKDEVICEPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

class OrgFreedesktopScreenSaverInterface;

#define PACKAGE_TYPE_LOCK QStringLiteral("kdeconnect.lock")
#define PACKAGE_TYPE_LOCK_REQUEST QStringLiteral("kdeconnect.lock.request")

class Q_DECL_EXPORT LockDevicePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.lockdevice")
    Q_PROPERTY(bool isLocked READ isLocked WRITE setLocked NOTIFY lockedChanged)

public:
    explicit LockDevicePlugin(QObject* parent, const QVariantList &args);
    ~LockDevicePlugin() override;

    bool isLocked() const;
    void setLocked(bool b);

    QString dbusPath() const override;
    void connected() override;
    bool receivePackage(const NetworkPackage & np) override;

Q_SIGNALS:
    void lockedChanged(bool locked);

private:
    bool m_remoteLocked;

    OrgFreedesktopScreenSaverInterface* iface();

    OrgFreedesktopScreenSaverInterface* m_iface;
};

#endif
