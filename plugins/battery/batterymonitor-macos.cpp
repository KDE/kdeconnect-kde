/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batterymonitor.h"

#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

class BatteryMonitor::Private
{
public:
    CFRunLoopSourceRef runLoopSource = nullptr;
};

static void powerSourcesChanged(void *context)
{
    // Called from the main run loop
    Q_EMIT static_cast<BatteryMonitor *>(context)->changed();
}

static bool boolValue(CFDictionaryRef dict, CFStringRef key)
{
    const auto value = static_cast<CFBooleanRef>(CFDictionaryGetValue(dict, key));
    return value && CFBooleanGetValue(value);
}

static int intValue(CFDictionaryRef dict, CFStringRef key)
{
    int result = 0;
    const auto value = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, key));
    if (value) {
        CFNumberGetValue(value, kCFNumberIntType, &result);
    }
    return result;
}

static bool stringEquals(CFDictionaryRef dict, CFStringRef key, CFStringRef expected)
{
    const auto value = static_cast<CFStringRef>(CFDictionaryGetValue(dict, key));
    return value && CFEqual(value, expected);
}

BatteryMonitor::BatteryMonitor(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->runLoopSource = IOPSNotificationCreateRunLoopSource(powerSourcesChanged, this);
    if (d->runLoopSource) {
        CFRunLoopAddSource(CFRunLoopGetMain(), d->runLoopSource, kCFRunLoopCommonModes);
    }
}

BatteryMonitor::~BatteryMonitor()
{
    if (d->runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), d->runLoopSource, kCFRunLoopCommonModes);
        CFRelease(d->runLoopSource);
    }
}

std::optional<BatteryStatus> BatteryMonitor::status() const
{
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    if (!info) {
        return std::nullopt;
    }
    CFArrayRef sources = IOPSCopyPowerSourcesList(info);
    if (!sources) {
        CFRelease(info);
        return std::nullopt;
    }

    bool isAnyBatteryCharging = false;
    bool isDischarging = false;
    int batteryQuantity = 0;
    int cumulativeCharge = 0;

    for (CFIndex i = 0; i < CFArrayGetCount(sources); i++) {
        CFDictionaryRef description = IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(sources, i));
        if (!description || !stringEquals(description, CFSTR(kIOPSTypeKey), CFSTR(kIOPSInternalBatteryType))
            || !boolValue(description, CFSTR(kIOPSIsPresentKey))) {
            continue;
        }

        const int maxCapacity = intValue(description, CFSTR(kIOPSMaxCapacityKey));
        if (maxCapacity <= 0) {
            continue;
        }

        batteryQuantity++;
        cumulativeCharge += intValue(description, CFSTR(kIOPSCurrentCapacityKey)) * 100 / maxCapacity;
        if (boolValue(description, CFSTR(kIOPSIsChargingKey))) {
            isAnyBatteryCharging = true;
        }
        if (stringEquals(description, CFSTR(kIOPSPowerSourceStateKey), CFSTR(kIOPSBatteryPowerValue))) {
            isDischarging = true;
        }
    }

    CFRelease(sources);
    CFRelease(info);

    if (batteryQuantity == 0) {
        return std::nullopt;
    }

    return BatteryStatus{
        .charge = cumulativeCharge / batteryQuantity,
        .isCharging = isAnyBatteryCharging,
        .isDischarging = isDischarging,
        .batteryQuantity = batteryQuantity,
    };
}

#include "moc_batterymonitor.cpp"
