#include "device.h"

Device::Device(const QString& id, const QString& name) {
    mDeviceId = id;
    mDeviceName = name;
    QDBusConnection::sessionBus().registerObject("module/androidshine/Devices/"+id, this);

}
