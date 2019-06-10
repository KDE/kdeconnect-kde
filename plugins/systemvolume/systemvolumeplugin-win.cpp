/**
 * Copyright 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
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

#include "systemvolumeplugin-win.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KPluginFactory>

#include <Functiondiscoverykeys_devpkey.h>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON(KdeConnectPluginFactory, "kdeconnect_systemvolume.json", registerPlugin<SystemvolumePlugin>();)

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SYSTEMVOLUME, "kdeconnect.plugin.systemvolume")

// Private classes of SystemvolumePlugin

class SystemvolumePlugin::CMMNotificationClient : public IMMNotificationClient
{

  public:
    CMMNotificationClient(SystemvolumePlugin &x) : enclosing(x), _cRef(1){};

    ~CMMNotificationClient(){};

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&_cRef);
        if (ulRef == 0)
        {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
        if (IID_IUnknown == riid)
        {
            AddRef();
            *ppvInterface = (IUnknown *)this;
        }
        else if (__uuidof(IMMNotificationClient) == riid)
        {
            AddRef();
            *ppvInterface = (IMMNotificationClient *)this;
        }
        else
        {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    // Callback methods for device-event notifications.

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) override
    {
        if (flow == eRender)
        {
            enclosing.sendSinkList();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override
    {
        enclosing.sendSinkList();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override
    {
        enclosing.sendSinkList();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
    {
        enclosing.sendSinkList();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override
    {
        enclosing.sendSinkList();
        return S_OK;
    }

  private:
    LONG _cRef;
    SystemvolumePlugin &enclosing;
};

class SystemvolumePlugin::CAudioEndpointVolumeCallback : public IAudioEndpointVolumeCallback
{
    LONG _cRef;

  public:
    CAudioEndpointVolumeCallback(SystemvolumePlugin &x, QString sinkName) : enclosing(x), name(sinkName), _cRef(1) {}
    ~CAudioEndpointVolumeCallback(){};

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ulRef = InterlockedDecrement(&_cRef);
        if (ulRef == 0)
        {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override
    {
        if (IID_IUnknown == riid)
        {
            AddRef();
            *ppvInterface = (IUnknown *)this;
        }
        else if (__uuidof(IMMNotificationClient) == riid)
        {
            AddRef();
            *ppvInterface = (IMMNotificationClient *)this;
        }
        else
        {
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

SystemvolumePlugin::SystemvolumePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args),
      sinkList()
{
    CoInitialize(nullptr);
    deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&(deviceEnumerator));
    valid = (hr == S_OK);
    if (!valid)
    {
        qWarning("Initialization failed: Failed to create MMDeviceEnumerator");
        qWarning("Error Code: %lx", hr);
    }
}

SystemvolumePlugin::~SystemvolumePlugin()
{
    if (valid)
    {
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

    HRESULT hr;
    if (!sinkList.empty())
    {
        for (auto const &sink : sinkList)
        {
            sink.first->UnregisterControlChangeNotify(sink.second);
            sink.first->Release();
            sink.second->Release();
        }
        sinkList.clear();
    }
    IMMDeviceCollection *devices = nullptr;
    hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);

    if (hr != S_OK)
    {
        qWarning("Failed to Enumumerate AudioEndpoints");
        qWarning("Error Code: %lx", hr);
        return false;
    }
    unsigned int deviceCount;
    devices->GetCount(&deviceCount);
    for (unsigned int i = 0; i < deviceCount; i++)
    {
        IMMDevice *device = nullptr;

        IPropertyStore *deviceProperties = nullptr;
        PROPVARIANT deviceProperty;
        QString name;
        QString desc;
        float volume;
        BOOL muted;

        IAudioEndpointVolume *endpoint = nullptr;
        CAudioEndpointVolumeCallback *callback;

        // Get Properties
        devices->Item(i, &device);
        device->OpenPropertyStore(STGM_READ, &deviceProperties);

        deviceProperties->GetValue(PKEY_Device_FriendlyName, &deviceProperty);
        name = QString::fromWCharArray(deviceProperty.pwszVal);
        //PropVariantClear(&deviceProperty);

#ifndef __MINGW32__
        deviceProperties->GetValue(PKEY_Device_DeviceDesc, &deviceProperty);
        desc = QString::fromWCharArray(deviceProperty.pwszVal);
        //PropVariantClear(&deviceProperty);
#endif

        QJsonObject sinkObject;
        sinkObject.insert(QStringLiteral("name"), name);
        sinkObject.insert(QStringLiteral("description"), desc);

        hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&endpoint);
        if (hr != S_OK)
        {
            qWarning() << "Failed to create IAudioEndpointVolume for device:" << name;
            qWarning("Error Code: %lx", hr);

            device->Release();
            continue;
        }
        endpoint->GetMasterVolumeLevelScalar(&volume);
        endpoint->GetMute(&muted);

        sinkObject.insert(QStringLiteral("muted"), (bool)muted);
        sinkObject.insert(QStringLiteral("volume"), (qint64)(volume * 100));
        sinkObject.insert(QStringLiteral("maxVolume"), (qint64)100);

        // Register Callback
        callback = new CAudioEndpointVolumeCallback(*this, name);
        sinkList[name] = qMakePair(endpoint, callback);
        endpoint->RegisterControlChangeNotify(callback);

        device->Release();
        array.append(sinkObject);
    }
    devices->Release();

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

bool SystemvolumePlugin::receivePacket(const NetworkPacket &np)
{
    if (!valid)
        return false;

    if (np.has(QStringLiteral("requestSinks")))
    {
        return sendSinkList();
    }
    else
    {
        QString name = np.get<QString>(QStringLiteral("name"));

        if (sinkList.contains(name))
        {
            if (np.has(QStringLiteral("volume")))
            {
                sinkList[name].first->SetMasterVolumeLevelScalar((float)np.get<int>(QStringLiteral("volume")) / 100, NULL);
            }
            if (np.has(QStringLiteral("muted")))
            {
                sinkList[name].first->SetMute(np.get<bool>(QStringLiteral("muted")), NULL);
            }
        }
    }
    return true;
}

#include "systemvolumeplugin-win.moc"
