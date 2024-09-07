/**
 * SPDX-FileCopyrightText: 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "systemvolumeplugin-win.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KPluginFactory>

#include <Functiondiscoverykeys_devpkey.h>

#include <core/device.h>

#include "PolicyConfig.h"
#include "plugin_systemvolume_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SystemvolumePlugin, "kdeconnect_systemvolume.json")

// Private classes of SystemvolumePlugin

class SystemvolumePlugin::CAudioEndpointVolumeCallback : public IAudioEndpointVolumeCallback
{
    LONG _cRef;

public:
    CAudioEndpointVolumeCallback(SystemvolumePlugin &x, QString sinkName)
        : enclosing(x)
        , name(std::move(sinkName))
        , _cRef(1)
    {
    }

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&_cRef);
        if (ulRef == 0) {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
        if (IID_IUnknown == riid) {
            AddRef();
            *ppvInterface = (IUnknown *)this;
        } else if (__uuidof(IMMNotificationClient) == riid) {
            AddRef();
            *ppvInterface = (IMMNotificationClient *)this;
        } else {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    // Callback method for endpoint-volume-change notifications.

    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override
    {
        NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
        np.set<int>(QStringLiteral("volume"), (int)(pNotify->fMasterVolume * 100));
        np.set<bool>(QStringLiteral("muted"), pNotify->bMuted);
        np.set<QString>(QStringLiteral("name"), name);
        enclosing.sendPacket(np);

        return S_OK;
    }

private:
    SystemvolumePlugin &enclosing;
    QString name;
};

class SystemvolumePlugin::CMMNotificationClient : public IMMNotificationClient
{
public:
    CMMNotificationClient(SystemvolumePlugin &x)
        : enclosing(x)
        , _cRef(1)
    {
    }

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&_cRef);
        if (ulRef == 0) {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
        if (IID_IUnknown == riid) {
            AddRef();
            *ppvInterface = (IUnknown *)this;
        } else if (__uuidof(IMMNotificationClient) == riid) {
            AddRef();
            *ppvInterface = (IMMNotificationClient *)this;
        } else {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    // Callback methods for device-event notifications.

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) override
    {
        if (flow == eRender) {
            enclosing.sendSinkList();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override
    {
        enclosing.sendSinkList();
        return S_OK;
    }

    struct RemovedDeviceThreadData {
        QString qDeviceId;
        SystemvolumePlugin *plugin;
    };

    static DWORD WINAPI releaseRemovedDevice(_In_ LPVOID lpParameter)
    {
        auto *data = static_cast<RemovedDeviceThreadData *>(lpParameter);

        if (!data->plugin->sinkList.empty()) {
            auto idToNameIterator = data->plugin->idToNameMap.find(data->qDeviceId);
            if (idToNameIterator == data->plugin->idToNameMap.end())
                return 0;

            QString &sinkName = idToNameIterator.value();

            auto sinkListIterator = data->plugin->sinkList.find(sinkName);
            if (sinkListIterator == data->plugin->sinkList.end())
                return 0;

            auto &sink = sinkListIterator.value();

            sink.first->UnregisterControlChangeNotify(sink.second);
            sink.first->Release();
            sink.second->Release();

            data->plugin->idToNameMap.remove(data->qDeviceId);
            data->plugin->sinkList.remove(sinkName);
        }

        data->plugin->sendSinkList();

        return 0;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override
    {
        static RemovedDeviceThreadData data{};
        data.qDeviceId = QString::fromWCharArray(pwstrDeviceId);
        data.plugin = &enclosing;

        DWORD threadId;
        HANDLE threadHandle = CreateThread(NULL, 0, releaseRemovedDevice, &data, 0, &threadId);
        CloseHandle(threadHandle);

        enclosing.sendSinkList();

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
    {
        if (dwNewState == DEVICE_STATE_UNPLUGGED)
            return OnDeviceRemoved(pwstrDeviceId);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override
    {
        // This callback is supper spammy. Care only about name and description changes.
        if (IsEqualPropertyKey(key, PKEY_Device_FriendlyName)) {
            enclosing.sendSinkList();
        }
#ifndef __MINGW32__
        else if (IsEqualPropertyKey(key, PKEY_Device_DeviceDesc)) {
            enclosing.sendSinkList();
        }
#endif
        return S_OK;
    }

private:
    LONG _cRef;
    SystemvolumePlugin &enclosing;
};

SystemvolumePlugin::SystemvolumePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , sinkList()
{
    CoInitialize(nullptr);
    deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&(deviceEnumerator));
    valid = (hr == S_OK);
    if (!valid) {
        qWarning("Initialization failed: Failed to create MMDeviceEnumerator");
        qWarning("Error Code: %lx", hr);
    }
}

SystemvolumePlugin::~SystemvolumePlugin()
{
    if (valid) {
        for (auto &entry : sinkList) {
            entry.first->UnregisterControlChangeNotify(entry.second);
            entry.second->Release();
            entry.first->Release();
        }
        deviceEnumerator->UnregisterEndpointNotificationCallback(deviceCallback);
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }
}

bool SystemvolumePlugin::sendSinkList()
{
    if (!valid)
        return false;

    QJsonDocument document;
    QJsonArray array;

    IMMDeviceCollection *devices = nullptr;
    HRESULT hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);

    if (hr != S_OK) {
        qWarning("Failed to Enumumerate AudioEndpoints");
        qWarning("Error Code: %lx", hr);
        return false;
    }
    unsigned int deviceCount;
    devices->GetCount(&deviceCount);

    if (!deviceCount) {
        qWarning("No audio devices detected");
        return false;
    }

    IMMDevice *defaultDevice = nullptr;
    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice);

    if (!defaultDevice) {
        qWarning("No default audio device detected");
        return false;
    }

    LPWSTR defaultId = NULL;
    defaultDevice->GetId(&defaultId);

    for (unsigned int i = 0; i < deviceCount; i++) {
        IMMDevice *device = nullptr;

        IPropertyStore *deviceProperties = nullptr;
        PROPVARIANT deviceProperty;
        QString name;
        QString desc;
        LPWSTR deviceId;
        float volume;
        BOOL muted;

        IAudioEndpointVolume *endpoint = nullptr;
        CAudioEndpointVolumeCallback *callback;

        // Get Properties
        devices->Item(i, &device);
        device->OpenPropertyStore(STGM_READ, &deviceProperties);

        deviceProperties->GetValue(PKEY_Device_FriendlyName, &deviceProperty);
        name = QString::fromWCharArray(deviceProperty.pwszVal);
        PropVariantClear(&deviceProperty);

#ifndef __MINGW32__
        deviceProperties->GetValue(PKEY_Device_DeviceDesc, &deviceProperty);
        desc = QString::fromWCharArray(deviceProperty.pwszVal);
        PropVariantClear(&deviceProperty);
#else
        desc = name;
#endif

        QJsonObject sinkObject;
        sinkObject.insert(QStringLiteral("name"), name);
        sinkObject.insert(QStringLiteral("description"), desc);

        hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&endpoint);
        if (hr != S_OK) {
            qWarning() << "Failed to create IAudioEndpointVolume for device:" << name;
            qWarning("Error Code: %lx", hr);

            device->Release();
            continue;
        }

        device->GetId(&deviceId);
        endpoint->GetMasterVolumeLevelScalar(&volume);
        endpoint->GetMute(&muted);

        sinkObject.insert(QStringLiteral("muted"), (bool)muted);
        sinkObject.insert(QStringLiteral("volume"), (qint64)(volume * 100));
        sinkObject.insert(QStringLiteral("maxVolume"), (qint64)100);
        sinkObject.insert(QStringLiteral("enabled"), lstrcmpW(deviceId, defaultId) == 0);

        // Register Callback
        if (!sinkList.contains(name)) {
            callback = new CAudioEndpointVolumeCallback(*this, name);
            endpoint->RegisterControlChangeNotify(callback);
            sinkList[name] = qMakePair(endpoint, callback);
        }

        QString qDeviceId = QString::fromWCharArray(deviceId);
        idToNameMap[qDeviceId] = name;

        device->Release();
        array.append(sinkObject);
    }
    devices->Release();
    defaultDevice->Release();

    document.setArray(array);

    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<QJsonDocument>(QStringLiteral("sinkList"), document);
    sendPacket(np);
    return true;
}

void SystemvolumePlugin::connected()
{
    if (!valid)
        return;

    deviceCallback = new CMMNotificationClient(*this);
    deviceEnumerator->RegisterEndpointNotificationCallback(deviceCallback);
    sendSinkList();
}

static HRESULT setDefaultAudioPlaybackDevice(PCWSTR deviceId)
{
    if (deviceId == nullptr)
        return ERROR_BAD_UNIT;

    IPolicyConfigVista *pPolicyConfig;
    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);

    if (SUCCEEDED(hr)) {
        hr = pPolicyConfig->SetDefaultEndpoint(deviceId, eMultimedia);
        pPolicyConfig->Release();
    }

    return hr;
}

HRESULT SystemvolumePlugin::setDefaultAudioPlaybackDevice(QString &name, bool enabled)
{
    if (!enabled)
        return S_OK;

    PWSTR deviceId = nullptr;
    for (auto &entry : idToNameMap.toStdMap()) {
        if (entry.second == name) {
            deviceId = new WCHAR[entry.first.length()];
            wcscpy(deviceId, entry.first.toStdWString().data());
            break;
        }
    }

    if (deviceId == nullptr)
        return ERROR_BAD_UNIT;

    HRESULT hr = ::setDefaultAudioPlaybackDevice(deviceId);

    delete[] deviceId;

    return hr;
}

void SystemvolumePlugin::receivePacket(const NetworkPacket &np)
{
    if (!valid)
        return;

    if (np.has(QStringLiteral("requestSinks"))) {
        sendSinkList();
    } else {
        QString name = np.get<QString>(QStringLiteral("name"));

        auto sinkListIterator = this->sinkList.find(name);
        if (sinkListIterator != this->sinkList.end()) {
            auto &sink = sinkListIterator.value();

            if (np.has(QStringLiteral("volume"))) {
                float requestedVolume = (float)np.get<int>(QStringLiteral("volume"), 100) / 100;
                sinkList[name].first->SetMasterVolumeLevelScalar(requestedVolume, NULL);
            }

            if (np.has(QStringLiteral("muted"))) {
                BOOL requestedMuteStatus = np.get<bool>(QStringLiteral("muted"), false);
                sinkList[name].first->SetMute(requestedMuteStatus, NULL);
            }

            if (np.has(QStringLiteral("enabled"))) {
                // get the current default device ID
                IMMDevice *defaultDevice = nullptr;
                deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice);
                LPWSTR defaultId = NULL;
                defaultDevice->GetId(&defaultId);
                defaultDevice->Release();
                // get current sink's device ID
                QString qDefaultId = QString::fromWCharArray(defaultId);
                QString currentDeviceId = idToNameMap.key(name);
                if (qDefaultId != currentDeviceId) {
                    setDefaultAudioPlaybackDevice(name, np.get<bool>(QStringLiteral("enabled")));
                }
            }
        }
    }
}

#include "moc_systemvolumeplugin-win.cpp"
#include "systemvolumeplugin-win.moc"
