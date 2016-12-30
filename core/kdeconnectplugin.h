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

#ifndef KDECONNECTPLUGIN_H
#define KDECONNECTPLUGIN_H

#include <QObject>
#include <QVariantList>

#include "kdeconnectcore_export.h"
#include "kdeconnectpluginconfig.h"
#include "networkpackage.h"
#include "device.h"

struct KdeConnectPluginPrivate;

class KDECONNECTCORE_EXPORT KdeConnectPlugin
    : public QObject
{
    Q_OBJECT

public:
    KdeConnectPlugin(QObject* parent, const QVariantList& args);
    ~KdeConnectPlugin() override;

    const Device* device();
    Device const* device() const;

    bool sendPackage(NetworkPackage& np) const;

    KdeConnectPluginConfig* config() const;

    virtual QString dbusPath() const;

public Q_SLOTS:
    /**
     * Returns true if it has handled the package in some way
     * device.sendPackage can be used to send an answer back to the device
     */
    virtual bool receivePackage(const NetworkPackage& np) = 0;

    /**
     * This method will be called when a device is connected to this computer.
     * The plugin could be loaded already, but there is no guarantee we will be able to reach the device until this is called.
     */
    virtual void connected() = 0;

private:
    QScopedPointer<KdeConnectPluginPrivate> d;

};

#endif
