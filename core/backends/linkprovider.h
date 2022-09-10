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
    const static int PRIORITY_LOW = 0; // eg: 3g internet
    const static int PRIORITY_MEDIUM = 50; // eg: internet
    const static int PRIORITY_HIGH = 100; // eg: lan

    LinkProvider();
    ~LinkProvider() override = default;

    virtual QString name() = 0;
    virtual int priority() = 0;

public Q_SLOTS:
    virtual void onStart() = 0;
    virtual void onStop() = 0;
    virtual void onNetworkChange() = 0;

    void suspend(bool suspend);

Q_SIGNALS:
    // NOTE: The provider will destroy the DeviceLink when it's no longer accessible,
    //       and every user should listen to the destroyed signal to remove its references.
    //       That's the reason because there is no "onConnectionLost".
    void onConnectionReceived(const NetworkPacket &identityPacket, DeviceLink *);
};

#endif
