/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LINKPROVIDER_H
#define LINKPROVIDER_H

#include <QObject>

#include "networkpacket.h"
#include "pairinghandler.h"

class DeviceLink;

class KDECONNECTCORE_EXPORT LinkProvider : public QObject
{
    Q_OBJECT

public:
    LinkProvider();

    virtual QString name() = 0;
    virtual int priority() = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;

public Q_SLOTS:
    virtual void onStart() = 0;
    virtual void onStop() = 0;
    virtual void onNetworkChange() = 0;
    virtual void onLinkDestroyed(const QString &deviceId, DeviceLink *oldPtr) = 0;

    void suspend(bool suspend);

Q_SIGNALS:
    void onConnectionReceived(DeviceLink *);
};

#endif
