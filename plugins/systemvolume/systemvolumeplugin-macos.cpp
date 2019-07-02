/**
 * Copyright 2019 Weixuan XIAO <veyx.shaw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "systemvolumeplugin-macos.h"

#include <KPluginFactory>

#include <QDebug>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_systemvolume.json", registerPlugin< SystemvolumePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SYSTEMVOLUME, "kdeconnect.plugin.systemvolume")

class MacOSCoreAudioDevice
{
private:
    AudioDeviceID deviceId;
    QString description;
    bool isStereo;

    friend class SystemvolumePlugin;
public:
    MacOSCoreAudioDevice(AudioDeviceID);
    ~MacOSCoreAudioDevice();

    void setVolume(float volume);
    float volume();
    void setMuted(bool muted);
    bool isMuted();

    void updateType();
};

static const AudioObjectPropertyAddress kAudioHardwarePropertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioStreamPropertyAddress = {
    kAudioDevicePropertyStreams,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioMasterVolumePropertyAddress = {
    kAudioDevicePropertyVolumeScalar,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioLeftVolumePropertyAddress = {
    kAudioDevicePropertyVolumeScalar,
    kAudioDevicePropertyScopeOutput,
    1};

static const AudioObjectPropertyAddress kAudioRightVolumePropertyAddress = {
    kAudioDevicePropertyVolumeScalar,
    kAudioDevicePropertyScopeOutput,
    2};

static const AudioObjectPropertyAddress kAudioMasterMutedPropertyAddress = {
    kAudioDevicePropertyMute,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioMasterDataSourcePropertyAddress = {
        kAudioDevicePropertyDataSource,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster};

OSStatus onVolumeChanged(AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    SystemvolumePlugin *plugin = (SystemvolumePlugin*)context;
    plugin->updateDeviceVolume(object);
    return noErr;
}

OSStatus onMutedChanged(AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    SystemvolumePlugin *plugin = (SystemvolumePlugin*)context;
    plugin->updateDeviceMuted(object);
    return noErr;
}

OSStatus onOutputSourceChanged(AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    SystemvolumePlugin *plugin = (SystemvolumePlugin*)context;
    plugin->sendSinkList();
    return noErr;
}

UInt32 getDeviceSourceId(AudioObjectID deviceId) {
    UInt32 dataSourceId;
    UInt32 size = sizeof(dataSourceId);
    OSStatus result = AudioObjectGetPropertyData(deviceId, &kAudioMasterDataSourcePropertyAddress, 0, NULL, &size, &dataSourceId);
    if (result != noErr)
        return kAudioDeviceUnknown;

    return dataSourceId;
}

QString translateDeviceSource(AudioObjectID deviceId) {
    UInt32 sourceId = getDeviceSourceId(deviceId);

    if (sourceId == kAudioDeviceUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unknown data source id of device" << deviceId;
        return QStringLiteral("");
    }

    CFStringRef sourceName = nullptr;
    AudioValueTranslation translation;
    translation.mInputData = &sourceId;
    translation.mInputDataSize = sizeof(sourceId);
    translation.mOutputData = &sourceName;
    translation.mOutputDataSize = sizeof(sourceName);

    UInt32 translationSize = sizeof(AudioValueTranslation);
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDataSourceNameForIDCFString,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster};

    OSStatus result = AudioObjectGetPropertyData(deviceId, &propertyAddress, 0, NULL, &translationSize, &translation);
    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Cannot get description of device" << deviceId;
        return QStringLiteral("");
    }

    QString ret = QString::fromCFString(sourceName);
    CFRelease(sourceName);

    return ret;
}

std::vector<AudioObjectID> GetAllOutputAudioDeviceIDs() {
    std::vector<AudioObjectID> outputDeviceIds;

    UInt32 size = 0;
    OSStatus result;

    result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &kAudioHardwarePropertyAddress, 0, NULL, &size);

    if (result != noErr) {
        qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME)
            << "Failed to read size of property " << kAudioHardwarePropertyDevices
            << " for device/object " << kAudioObjectSystemObject;
        return {};
    }

    if (size == 0)
        return {};

    size_t device_count = size / sizeof(AudioObjectID);
    std::vector<AudioObjectID> deviceIds(device_count);
    result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &kAudioHardwarePropertyAddress, 0, NULL, &size, deviceIds.data());
    if (result != noErr) {
        qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME)
            << "Failed to read object IDs from property " << kAudioHardwarePropertyDevices
            << " for device/object " << kAudioObjectSystemObject;
        return {};
    }

    for (AudioDeviceID deviceId : deviceIds) {
        UInt32 streamCount = 0;
        result = AudioObjectGetPropertyDataSize(deviceId, &kAudioStreamPropertyAddress, 0, NULL, &streamCount);

        if (result == noErr && streamCount > 0) {
            outputDeviceIds.push_back(deviceId);
            qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Device" << deviceId << "added";
        } else {
            qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Device" << deviceId << "dropped";
        }
    }

    return outputDeviceIds;
}

SystemvolumePlugin::SystemvolumePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , sinksMap()
{}

bool SystemvolumePlugin::receivePacket(const NetworkPacket& np)
{
    if (np.has(QStringLiteral("requestSinks"))) {
        sendSinkList();
    } else {
        QString name = np.get<QString>(QStringLiteral("name"));

        if (sinksMap.contains(name)) {
            if (np.has(QStringLiteral("volume"))) {
                sinksMap[name]->setVolume(np.get<int>(QStringLiteral("volume")) / 100.0);
            }
            if (np.has(QStringLiteral("muted"))) {
                sinksMap[name]->setMuted(np.get<bool>(QStringLiteral("muted")));
            }
        }
    }

    return true;
}

void SystemvolumePlugin::sendSinkList()
{
    QJsonDocument document;
    QJsonArray array;

    if (!sinksMap.empty()) {
        for (MacOSCoreAudioDevice *sink : sinksMap) {
            delete sink;
        }
        sinksMap.clear();
    }

    std::vector<AudioObjectID> deviceIds = GetAllOutputAudioDeviceIDs();

    for (AudioDeviceID deviceId : deviceIds) {
        MacOSCoreAudioDevice *audioDevice = new MacOSCoreAudioDevice(deviceId);

        audioDevice->description = translateDeviceSource(deviceId);

        sinksMap.insert(QStringLiteral("default-") + QString::number(deviceId), audioDevice);

        // Add volume change listener
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterVolumePropertyAddress, &onVolumeChanged, (void *)this);

        AudioObjectAddPropertyListener(deviceId, &kAudioLeftVolumePropertyAddress, &onVolumeChanged, (void *)this);
        AudioObjectAddPropertyListener(deviceId, &kAudioRightVolumePropertyAddress, &onVolumeChanged, (void *)this);

        // Add muted change listener
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterMutedPropertyAddress, &onMutedChanged, (void *)this);

        // Add data source change listerner
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterDataSourcePropertyAddress, &onOutputSourceChanged, (void *)this);

        QJsonObject sinkObject {
            {QStringLiteral("name"), QStringLiteral("default-") + QString::number(deviceId)},
            {QStringLiteral("muted"), audioDevice->isMuted()},
            {QStringLiteral("description"), audioDevice->description},
            {QStringLiteral("volume"), audioDevice->volume() * 100},
            {QStringLiteral("maxVolume"), 100}
        };

        array.append(sinkObject);
    }

    document.setArray(array);

    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<QJsonDocument>(QStringLiteral("sinkList"), document);
    sendPacket(np);
}

void SystemvolumePlugin::connected()
{
    sendSinkList();
}

void SystemvolumePlugin::updateDeviceMuted(AudioDeviceID deviceId)
{
    for (MacOSCoreAudioDevice *sink : sinksMap) {
        if (sink->deviceId == deviceId) {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<bool>(QStringLiteral("muted"), (bool)(sink->isMuted()));
            np.set<int>(QStringLiteral("volume"), (int)(sink->volume() * 100));
            np.set<QString>(QStringLiteral("name"), QStringLiteral("default-") + QString::number(deviceId));
            sendPacket(np);
            return;
        }
    }
    qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Device" << deviceId << "not found while update mute state";
}

void SystemvolumePlugin::updateDeviceVolume(AudioDeviceID deviceId)
{
    for (MacOSCoreAudioDevice *sink : sinksMap) {
        if (sink->deviceId == deviceId) {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<int>(QStringLiteral("volume"), (int)(sink->volume() * 100));
            np.set<QString>(QStringLiteral("name"), QStringLiteral("default-") + QString::number(deviceId));
            sendPacket(np);
            return;
        }
    }
    qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Device" << deviceId << "not found while update volume";
}

MacOSCoreAudioDevice::MacOSCoreAudioDevice(AudioDeviceID _deviceId)
    : deviceId(_deviceId)
{
    updateType();
}

MacOSCoreAudioDevice::~MacOSCoreAudioDevice()
{
    // Volume listener
    AudioObjectRemovePropertyListener(deviceId, &kAudioMasterVolumePropertyAddress,
        &onVolumeChanged, (void *)this);
    AudioObjectRemovePropertyListener(deviceId, &kAudioLeftVolumePropertyAddress,
        &onVolumeChanged, (void *)this);
    AudioObjectRemovePropertyListener(deviceId, &kAudioRightVolumePropertyAddress,
        &onVolumeChanged, (void *)this);

    // Muted listener
    AudioObjectRemovePropertyListener(deviceId, &kAudioMasterMutedPropertyAddress,
        &onMutedChanged, (void *)this);

    // Data source listener
    AudioObjectRemovePropertyListener(deviceId, &kAudioMasterDataSourcePropertyAddress,
        &onOutputSourceChanged, (void *)this);
}

void MacOSCoreAudioDevice::setVolume(float volume)
{
    OSStatus result;

    if (deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set volume of Unknown Device";
        return;
    }

    if (isStereo) {
        result = AudioObjectSetPropertyData(deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
        result = AudioObjectSetPropertyData(deviceId, &kAudioRightVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
    } else {
        result = AudioObjectSetPropertyData(deviceId, &kAudioMasterVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
    }

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set volume of Device" << deviceId << "to" << volume;
    }
}

void MacOSCoreAudioDevice::setMuted(bool muted)
{
    if (deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to mute an Unknown Device";
        return;
    }

    UInt32 mutedValue = muted ? 1 : 0;

    OSStatus result = AudioObjectSetPropertyData(deviceId, &kAudioMasterMutedPropertyAddress, 0, NULL, sizeof(mutedValue), &mutedValue);

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set muted state of Device" << deviceId << "to" << muted;
    }
}

float MacOSCoreAudioDevice::volume()
{
    OSStatus result;

    if (deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get volume of Unknown Device";
        return 0.0;
    }

    float volume = 0.0;
    UInt32 volumeDataSize = sizeof(volume);

    if (isStereo) {
        // Try to get steoreo device volume
        result = AudioObjectGetPropertyData(deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    } else {
        // Try to get master volume
        result = AudioObjectGetPropertyData(deviceId, &kAudioMasterVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    }

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get volume of Device" << deviceId;
        return 0.0;
    }

    return volume;
}

bool MacOSCoreAudioDevice::isMuted()
{
    if (deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get muted state of an Unknown Device";
        return false;
    }

    UInt32 muted = 0;
    UInt32 muteddataSize = sizeof(muted);

    AudioObjectGetPropertyData(deviceId, &kAudioMasterMutedPropertyAddress, 0, NULL, &muteddataSize, &muted);

    return muted == 1;
}

void MacOSCoreAudioDevice::updateType()
{
    // Try to get volume from left channel to check if it's a stereo device
    float volume = 0.0;
    UInt32 volumeDataSize = sizeof(volume);
    OSStatus result = AudioObjectGetPropertyData(deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    if (result == noErr) {
        isStereo = true;
    } else {
        isStereo = false;
    }
}


#include "systemvolumeplugin-macos.moc"

