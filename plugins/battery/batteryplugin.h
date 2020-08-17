/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BATTERYPLUGIN_H
#define BATTERYPLUGIN_H

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_BATTERY QStringLiteral("kdeconnect.battery")
#define PACKET_TYPE_BATTERY_REQUEST QStringLiteral("kdeconnect.battery.request")

class BatteryDbusInterface;

class BatteryPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit BatteryPlugin(QObject* parent, const QVariantList& args);
    ~BatteryPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    // NB: This may be connected to zero or two signals in Solid::Battery -
    // chargePercentageChanged and chargeStatusChanged.
    // See inline comments for further details
    void chargeChanged();

private:
    // Keep these values in sync with THRESHOLD* constants in
    // kdeconnect-android:BatteryPlugin.java
    // see README for their meaning
    enum ThresholdBatteryEvent {
        ThresholdNone       = 0,
        ThresholdBatteryLow = 1
    };

    BatteryDbusInterface* batteryDbusInterface;
};

#endif
