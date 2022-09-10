/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECTPLUGIN_H
#define KDECONNECTPLUGIN_H

#include <QObject>
#include <QVariantList>
#include <kcoreaddons_version.h>

#include "device.h"
#include "kdeconnectcore_export.h"
#include "kdeconnectpluginconfig.h"
#include "networkpacket.h"

#if KCOREADDONS_VERSION < QT_VERSION_CHECK(5, 44, 0)
#define K_PLUGIN_CLASS_WITH_JSON(classname, jsonFile) K_PLUGIN_FACTORY_WITH_JSON(classname##Factory, jsonFile, registerPlugin<classname>();)
#endif

struct KdeConnectPluginPrivate;

class KDECONNECTCORE_EXPORT KdeConnectPlugin : public QObject
{
    Q_OBJECT

public:
    KdeConnectPlugin(QObject *parent, const QVariantList &args);
    ~KdeConnectPlugin() override;

    const Device *device();
    Device const *device() const;

    bool sendPacket(NetworkPacket &np) const;

    KdeConnectPluginConfig *config() const;

    virtual QString dbusPath() const;

    QString iconName() const;

public Q_SLOTS:
    /**
     * Returns true if it has handled the packet in some way
     * device.sendPacket can be used to send an answer back to the device
     */
    virtual bool receivePacket(const NetworkPacket &np) = 0;

    /**
     * This method will be called when a device is connected to this computer.
     * The plugin could be loaded already, but there is no guarantee we will be able to reach the device until this is called.
     */
    virtual void connected() = 0;

private:
    QScopedPointer<KdeConnectPluginPrivate> d;
};

#endif
