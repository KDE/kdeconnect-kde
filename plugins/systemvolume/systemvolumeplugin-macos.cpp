/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "systemvolumeplugin-macos.h"

#include <KPluginFactory>

#include "plugin_systemvolume_debug.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

K_PLUGIN_CLASS_WITH_JSON(SystemvolumePlugin, "kdeconnect_systemvolume.json")

class MacOSCoreAudioDevice
{
private:
    AudioDeviceID m_deviceId;
    QString m_description;
    bool m_isStereo;

    friend class SystemvolumePlugin;

public:
    MacOSCoreAudioDevice(AudioDeviceID);
    ~MacOSCoreAudioDevice();

    void setVolume(float volume);
    float volume();
    void setMuted(bool muted);
    bool isMuted();
    void setDefault(bool enabled);
    bool isDefault();

    void updateType();
};

static const AudioObjectPropertyAddress kAudioHardwarePropertyAddress = {kAudioHardwarePropertyDevices,
                                                                         kAudioObjectPropertyScopeGlobal,
                                                                         kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioStreamPropertyAddress = {kAudioDevicePropertyStreams,
                                                                       kAudioDevicePropertyScopeOutput,
                                                                       kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioMasterVolumePropertyAddress = {kAudioDevicePropertyVolumeScalar,
                                                                             kAudioDevicePropertyScopeOutput,
                                                                             kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioLeftVolumePropertyAddress = {kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, 1};

static const AudioObjectPropertyAddress kAudioRightVolumePropertyAddress = {kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, 2};

static const AudioObjectPropertyAddress kAudioMasterMutedPropertyAddress = {kAudioDevicePropertyMute,
                                                                            kAudioDevicePropertyScopeOutput,
                                                                            kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioMasterDataSourcePropertyAddress = {kAudioDevicePropertyDataSource,
                                                                                 kAudioDevicePropertyScopeOutput,
                                                                                 kAudioObjectPropertyElementMaster};

static const AudioObjectPropertyAddress kAudioDefaultOutputDevicePropertyAddress = {kAudioHardwarePropertyDefaultOutputDevice,
                                                                                    kAudioObjectPropertyScopeGlobal,
                                                                                    kAudioObjectPropertyElementMaster};

OSStatus onVolumeChanged(AudioObjectID object, UInt32 numAddresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    Q_UNUSED(object);
    Q_UNUSED(addresses);
    Q_UNUSED(numAddresses);

    SystemvolumePlugin *plugin = (SystemvolumePlugin *)context;
    plugin->updateDeviceVolume(object);
    return noErr;
}

OSStatus onMutedChanged(AudioObjectID object, UInt32 numAddresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    Q_UNUSED(object);
    Q_UNUSED(addresses);
    Q_UNUSED(numAddresses);

    SystemvolumePlugin *plugin = (SystemvolumePlugin *)context;
    plugin->updateDeviceMuted(object);
    return noErr;
}

OSStatus onDefaultChanged(AudioObjectID object, UInt32 numAddresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    Q_UNUSED(object);
    Q_UNUSED(addresses);
    Q_UNUSED(numAddresses);

    SystemvolumePlugin *plugin = (SystemvolumePlugin *)context;
    plugin->sendSinkList();
    return noErr;
}

OSStatus onOutputSourceChanged(AudioObjectID object, UInt32 numAddresses, const AudioObjectPropertyAddress addresses[], void *context)
{
    Q_UNUSED(object);
    Q_UNUSED(addresses);
    Q_UNUSED(numAddresses);

    SystemvolumePlugin *plugin = (SystemvolumePlugin *)context;
    plugin->sendSinkList();
    return noErr;
}

AudioObjectID getDefaultOutputDeviceId()
{
    AudioObjectID dataSourceId;
    UInt32 size = sizeof(dataSourceId);
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &kAudioDefaultOutputDevicePropertyAddress, 0, NULL, &size, &dataSourceId);
    if (result != noErr)
        return kAudioDeviceUnknown;

    return dataSourceId;
}

UInt32 getDeviceSourceId(AudioObjectID deviceId)
{
    UInt32 dataSourceId;
    UInt32 size = sizeof(dataSourceId);
    OSStatus result = AudioObjectGetPropertyData(deviceId, &kAudioMasterDataSourcePropertyAddress, 0, NULL, &size, &dataSourceId);
    if (result != noErr)
        return kAudioDeviceUnknown;

    return dataSourceId;
}

QString translateDeviceSource(AudioObjectID deviceId)
{
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
    AudioObjectPropertyAddress propertyAddress = {kAudioDevicePropertyDataSourceNameForIDCFString,
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

std::vector<AudioObjectID> GetAllOutputAudioDeviceIDs()
{
    std::vector<AudioObjectID> outputDeviceIds;

    UInt32 size = 0;
    OSStatus result;

    result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &kAudioHardwarePropertyAddress, 0, NULL, &size);

    if (result != noErr) {
        qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Failed to read size of property " << kAudioHardwarePropertyDevices << " for device/object "
                                                << kAudioObjectSystemObject;
        return {};
    }

    if (size == 0)
        return {};

    size_t deviceCount = size / sizeof(AudioObjectID);
    std::vector<AudioObjectID> deviceIds(deviceCount);
    result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &kAudioHardwarePropertyAddress, 0, NULL, &size, deviceIds.data());
    if (result != noErr) {
        qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Failed to read object IDs from property " << kAudioHardwarePropertyDevices << " for device/object "
                                                << kAudioObjectSystemObject;
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

SystemvolumePlugin::SystemvolumePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_sinksMap()
{
}

SystemvolumePlugin::~SystemvolumePlugin()
{
    AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &kAudioDefaultOutputDevicePropertyAddress, &onDefaultChanged, (void *)this);
}

void SystemvolumePlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("requestSinks"))) {
        sendSinkList();
    } else {
        QString name = np.get<QString>(QStringLiteral("name"));

        if (m_sinksMap.contains(name)) {
            if (np.has(QStringLiteral("volume"))) {
                m_sinksMap[name]->setVolume(np.get<int>(QStringLiteral("volume")) / 100.0);
            }
            if (np.has(QStringLiteral("muted"))) {
                m_sinksMap[name]->setMuted(np.get<bool>(QStringLiteral("muted")));
            }
            if (np.has(QStringLiteral("enabled"))) {
                m_sinksMap[name]->setDefault(np.get<bool>(QStringLiteral("enabled")));
            }
        }
    }
}

void SystemvolumePlugin::sendSinkList()
{
    QJsonDocument document;
    QJsonArray array;

    if (!m_sinksMap.empty()) {
        for (MacOSCoreAudioDevice *sink : m_sinksMap) {
            delete sink;
        }
        m_sinksMap.clear();
    }

    std::vector<AudioObjectID> deviceIds = GetAllOutputAudioDeviceIDs();

    for (AudioDeviceID deviceId : deviceIds) {
        MacOSCoreAudioDevice *audioDevice = new MacOSCoreAudioDevice(deviceId);

        audioDevice->m_description = translateDeviceSource(deviceId);

        m_sinksMap.insert(QStringLiteral("default-") + QString::number(deviceId), audioDevice);

        // Add volume change listener
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterVolumePropertyAddress, &onVolumeChanged, (void *)this);

        AudioObjectAddPropertyListener(deviceId, &kAudioLeftVolumePropertyAddress, &onVolumeChanged, (void *)this);
        AudioObjectAddPropertyListener(deviceId, &kAudioRightVolumePropertyAddress, &onVolumeChanged, (void *)this);

        // Add muted change listener
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterMutedPropertyAddress, &onMutedChanged, (void *)this);

        // Add data source change listerner
        AudioObjectAddPropertyListener(deviceId, &kAudioMasterDataSourcePropertyAddress, &onOutputSourceChanged, (void *)this);

        QString name = QStringLiteral("default-") + QString::number(deviceId);
        QJsonObject sinkObject{{QStringLiteral("name"), name},
                               {QStringLiteral("muted"), audioDevice->isMuted()},
                               {QStringLiteral("description"), audioDevice->m_description},
                               {QStringLiteral("volume"), audioDevice->volume() * 100},
                               {QStringLiteral("maxVolume"), 100},
                               {QStringLiteral("enabled"), audioDevice->isDefault()}};

        array.append(sinkObject);
    }

    document.setArray(array);

    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<QJsonDocument>(QStringLiteral("sinkList"), document);
    sendPacket(np);
}

void SystemvolumePlugin::connected()
{
    AudioObjectAddPropertyListener(kAudioObjectSystemObject, &kAudioDefaultOutputDevicePropertyAddress, &onDefaultChanged, (void *)this);
    sendSinkList();
}

void SystemvolumePlugin::updateDeviceMuted(AudioDeviceID deviceId)
{
    for (MacOSCoreAudioDevice *sink : m_sinksMap) {
        if (sink->m_deviceId == deviceId) {
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
    for (MacOSCoreAudioDevice *sink : m_sinksMap) {
        if (sink->m_deviceId == deviceId) {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<int>(QStringLiteral("volume"), (int)(sink->volume() * 100));
            np.set<QString>(QStringLiteral("name"), QStringLiteral("default-") + QString::number(deviceId));
            sendPacket(np);
            return;
        }
    }
    qCDebug(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Device" << deviceId << "not found while update volume";
}

MacOSCoreAudioDevice::MacOSCoreAudioDevice(AudioDeviceID deviceId)
    : m_deviceId(deviceId)
{
    updateType();
}

MacOSCoreAudioDevice::~MacOSCoreAudioDevice()
{
    // Volume listener
    AudioObjectRemovePropertyListener(m_deviceId, &kAudioMasterVolumePropertyAddress, &onVolumeChanged, (void *)this);
    AudioObjectRemovePropertyListener(m_deviceId, &kAudioLeftVolumePropertyAddress, &onVolumeChanged, (void *)this);
    AudioObjectRemovePropertyListener(m_deviceId, &kAudioRightVolumePropertyAddress, &onVolumeChanged, (void *)this);

    // Muted listener
    AudioObjectRemovePropertyListener(m_deviceId, &kAudioMasterMutedPropertyAddress, &onMutedChanged, (void *)this);

    // Data source listener
    AudioObjectRemovePropertyListener(m_deviceId, &kAudioMasterDataSourcePropertyAddress, &onOutputSourceChanged, (void *)this);
}

void MacOSCoreAudioDevice::setVolume(float volume)
{
    OSStatus result;

    if (m_deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set volume of Unknown Device";
        return;
    }

    if (m_isStereo) {
        result = AudioObjectSetPropertyData(m_deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
        result = AudioObjectSetPropertyData(m_deviceId, &kAudioRightVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
    } else {
        result = AudioObjectSetPropertyData(m_deviceId, &kAudioMasterVolumePropertyAddress, 0, NULL, sizeof(volume), &volume);
    }

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set volume of Device" << m_deviceId << "to" << volume;
    }
}

void MacOSCoreAudioDevice::setMuted(bool muted)
{
    if (m_deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to mute an Unknown Device";
        return;
    }

    UInt32 mutedValue = muted ? 1 : 0;

    OSStatus result = AudioObjectSetPropertyData(m_deviceId, &kAudioMasterMutedPropertyAddress, 0, NULL, sizeof(mutedValue), &mutedValue);

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set muted state of Device" << m_deviceId << "to" << muted;
    }
}

void MacOSCoreAudioDevice::setDefault(bool enabled)
{
    if (!enabled)
        return;

    if (m_deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set an Unknown Device as default output";
        return;
    }

    OSStatus result = AudioObjectSetPropertyData(kAudioObjectSystemObject, &kAudioDefaultOutputDevicePropertyAddress, 0, NULL, sizeof(m_deviceId), &m_deviceId);

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to set default state of Device" << m_deviceId;
    }
}

float MacOSCoreAudioDevice::volume()
{
    OSStatus result;

    if (m_deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get volume of Unknown Device";
        return 0.0;
    }

    float volume = 0.0;
    UInt32 volumeDataSize = sizeof(volume);

    if (m_isStereo) {
        // Try to get steoreo device volume
        result = AudioObjectGetPropertyData(m_deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    } else {
        // Try to get master volume
        result = AudioObjectGetPropertyData(m_deviceId, &kAudioMasterVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    }

    if (result != noErr) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get volume of Device" << m_deviceId;
        return 0.0;
    }

    return volume;
}

bool MacOSCoreAudioDevice::isMuted()
{
    if (m_deviceId == kAudioObjectUnknown) {
        qWarning(KDECONNECT_PLUGIN_SYSTEMVOLUME) << "Unable to get muted state of an Unknown Device";
        return false;
    }

    UInt32 muted = 0;
    UInt32 muteddataSize = sizeof(muted);

    AudioObjectGetPropertyData(m_deviceId, &kAudioMasterMutedPropertyAddress, 0, NULL, &muteddataSize, &muted);

    return muted == 1;
}

bool MacOSCoreAudioDevice::isDefault()
{
    AudioObjectID defaultDeviceId = getDefaultOutputDeviceId();
    return m_deviceId == defaultDeviceId;
}

void MacOSCoreAudioDevice::updateType()
{
    // Try to get volume from left channel to check if it's a stereo device
    float volume = 0.0;
    UInt32 volumeDataSize = sizeof(volume);
    OSStatus result = AudioObjectGetPropertyData(m_deviceId, &kAudioLeftVolumePropertyAddress, 0, NULL, &volumeDataSize, &volume);
    if (result == noErr) {
        m_isStereo = true;
    } else {
        m_isStereo = false;
    }
}

#include "moc_systemvolumeplugin-macos.cpp"
#include "systemvolumeplugin-macos.moc"
