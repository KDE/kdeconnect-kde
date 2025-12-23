/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ASYNCLINKPROVIDER_H
#define ASYNCLINKPROVIDER_H

#include "../linkprovider.h"
#include "bluetoothlinkprovider.h"
#include <QMutex>
#include <QPointer>
#include <QQueue>
#include <QThread>
#include <QTimer>

enum AsyncLinkproviderEvents {
    CONSTRUCTOR,
    ENABLE,
    DISABLE,
    START,
    STOP,
    NETWORKCHANGE
};

class AsyncLinkProvider : public LinkProvider
{
    Q_OBJECT
public:
    AsyncLinkProvider();
    ~AsyncLinkProvider() override;

    AsyncLinkProvider(bool isDisabled);

    QString name() override;
    QString displayName() override;

    int priority() override;
    void enable() override;
    void disable() override;
    void onStart() override;
    void onStop() override;
    void onNetworkChange() override;
    void deviceRemoved(const QString &deviceId) override;
    void runWorker();

private:
    BluetoothLinkProvider *wrappedInstance;
    QQueue<AsyncLinkproviderEvents> events;
    QMutex eventsMutex;
    bool startDisabled;
    QThread workerThread;
    QTimer *connectTimer;

private Q_SLOTS:
    void onNewDeviceLink(DeviceLink *link);
};

#endif
