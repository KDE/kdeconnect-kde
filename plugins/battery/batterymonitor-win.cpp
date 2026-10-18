/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batterymonitor.h"

// initguid.h makes windows.h define (not just declare) the GUID_* power settings
#include <initguid.h>

#include <windows.h>

#include <powersetting.h>
#include <powrprof.h>

class BatteryMonitor::Private
{
public:
    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS subscribeParameters = {};
    HPOWERNOTIFY percentageNotification = nullptr;
    HPOWERNOTIFY powerSourceNotification = nullptr;
};

static ULONG CALLBACK powerSettingChanged(PVOID context, ULONG, PVOID)
{
    // Called from a system thread
    QMetaObject::invokeMethod(static_cast<BatteryMonitor *>(context), &BatteryMonitor::changed, Qt::QueuedConnection);
    return ERROR_SUCCESS;
}

BatteryMonitor::BatteryMonitor(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->subscribeParameters.Callback = powerSettingChanged;
    d->subscribeParameters.Context = this;
    PowerSettingRegisterNotification(&GUID_BATTERY_PERCENTAGE_REMAINING, DEVICE_NOTIFY_CALLBACK, &d->subscribeParameters, &d->percentageNotification);
    PowerSettingRegisterNotification(&GUID_ACDC_POWER_SOURCE, DEVICE_NOTIFY_CALLBACK, &d->subscribeParameters, &d->powerSourceNotification);
}

BatteryMonitor::~BatteryMonitor()
{
    if (d->percentageNotification) {
        PowerSettingUnregisterNotification(d->percentageNotification);
    }
    if (d->powerSourceNotification) {
        PowerSettingUnregisterNotification(d->powerSourceNotification);
    }
}

std::optional<BatteryStatus> BatteryMonitor::status() const
{
    SYSTEM_POWER_STATUS powerStatus;
    if (!GetSystemPowerStatus(&powerStatus)) {
        return std::nullopt;
    }

    // BATTERY_FLAG_UNKNOWN (255) also has the BATTERY_FLAG_NO_BATTERY bit set
    if ((powerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY) || powerStatus.BatteryLifePercent == BATTERY_PERCENTAGE_UNKNOWN) {
        return std::nullopt;
    }

    return BatteryStatus{
        .charge = powerStatus.BatteryLifePercent,
        .isCharging = (powerStatus.BatteryFlag & BATTERY_FLAG_CHARGING) != 0,
        .isDischarging = powerStatus.ACLineStatus == AC_LINE_OFFLINE,
        .batteryQuantity = 1,
    };
}

#include "moc_batterymonitor.cpp"
