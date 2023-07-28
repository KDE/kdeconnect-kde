/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 * SPDX-FileCopyrightText: 2019 Richard Liebscher <richard.liebscher@gmail.com>
 * SPDX-FileCopyrightText: 2022 Yoram Bar Haim <bhyoram@protonmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KPluginFactory>
#include <ModemManagerQt/Call>
#include <ModemManagerQt/Manager>
#include <ModemManagerQt/Modem>
#include <ModemManagerQt/ModemVoice>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QVariant>

#include <core/kdeconnectplugin.h>

class QDBusPendingCallWatcher;
class QDBusInterface;
class QDBusError;
class QDBusObjectPath;
class QDBusVariant;

class MMTelephonyPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony")

public:
    explicit MMTelephonyPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;

private:
    void onCallAdded(ModemManager::Call::Ptr call);
    void onCallRemoved(ModemManager::Call::Ptr call);
    void onModemAdded(const QString &path);
    void onCallStateChanged(ModemManager::Call *call, MMCallState newState, MMCallState oldState);
    void sendMMTelephonyPacket(ModemManager::Call *call, const QString &state);
    void sendCancelMMTelephonyPacket(ModemManager::Call *call, const QString &lastState);
    static QString stateName(MMCallState state);
};
