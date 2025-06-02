/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pausemusicplugin-win.h"

#include "plugin_pausemusic_debug.h"
#include <KPluginFactory>

#include <Functiondiscoverykeys_devpkey.h>

K_PLUGIN_CLASS_WITH_JSON(PauseMusicPlugin, "kdeconnect_pausemusic.json")

PauseMusicPlugin::PauseMusicPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    CoInitialize(nullptr);
    deviceEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    valid = (hr == S_OK);
    if (!valid) {
        qWarning("Initialization failed: Failed to create MMDeviceEnumerator");
        qWarning("Error Code: %lx", hr);
    }
}

bool PauseMusicPlugin::updateSinksList()
{
    sinksList.clear();
    if (!valid)
        return false;

    IMMDeviceCollection *devices = nullptr;
    HRESULT hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);

    if (hr != S_OK) {
        qWarning("Failed to Enumumerate AudioEndpoints");
        qWarning("Error Code: %lx", hr);
        return false;
    }

    unsigned int deviceCount;
    devices->GetCount(&deviceCount);

    for (unsigned int i = 0; i < deviceCount; i++) {
        IMMDevice *device = nullptr;

        IPropertyStore *deviceProperties = nullptr;
        PROPVARIANT deviceProperty;
        QString name;

        IAudioEndpointVolume *endpoint = nullptr;

        // Get Properties
        devices->Item(i, &device);
        device->OpenPropertyStore(STGM_READ, &deviceProperties);

        deviceProperties->GetValue(PKEY_Device_FriendlyName, &deviceProperty);
        name = QString::fromWCharArray(deviceProperty.pwszVal);
        // PropVariantClear(&deviceProperty);

        hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void **)&endpoint);
        if (hr != S_OK) {
            qWarning() << "Failed to create IAudioEndpointVolume for device:" << name;
            qWarning("Error Code: %lx", hr);

            device->Release();
            continue;
        }

        // Register Callback
        if (!sinksList.contains(name)) {
            sinksList[name] = endpoint;
        }
        device->Release();
    }
    devices->Release();
    return true;
}

void PauseMusicPlugin::updatePlayersList()
{
    playersList.clear();
    try {
        auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get().GetSessions();
        for (uint32_t i = 0; i < sessions.Size(); i++) {
            const auto player = sessions.GetAt(i);
            auto playerName = player.SourceAppUserModelId();

            QString uniqueName = QString::fromWCharArray(playerName.c_str());
            for (int i = 2; playersList.contains(uniqueName); ++i) {
                uniqueName += QStringLiteral(" [") + QString::number(i) + QStringLiteral("]");
            }
            playersList.insert(uniqueName, player);
        }
    } catch (winrt::hresult_error e) {
        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Failed to update player list";
    }
}

PauseMusicPlugin::~PauseMusicPlugin()
{
    CoUninitialize();
}

void PauseMusicPlugin::receivePacket(const NetworkPacket &np)
{
    bool pauseOnlyWhenTalking = config()->getBool(QStringLiteral("conditionTalking"), false);

    if (pauseOnlyWhenTalking) {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return;
        }
    } else {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("ringing") && np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return;
        }
    }

    bool pauseConditionFulfilled = !np.get<bool>(QStringLiteral("isCancel"));

    bool pause = config()->getBool(QStringLiteral("actionPause"), true);
    bool mute = config()->getBool(QStringLiteral("actionMute"), false);

    const bool autoResume = config()->getBool(QStringLiteral("actionResume"), true);

    if (pauseConditionFulfilled) {
        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Muting all the unmuted sinks";
            this->updateSinksList();
            QHashIterator<QString, IAudioEndpointVolume *> sinksIterator(sinksList);
            while (sinksIterator.hasNext()) {
                sinksIterator.next();
                BOOL muted;
                sinksIterator.value()->GetMute(&muted);
                if (!((bool)muted)) {
                    qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Trying to mute " << sinksIterator.key();
                    if (sinksIterator.value()->SetMute(true, NULL) == S_OK) {
                        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Muted " << sinksIterator.key();
                        mutedSinks.insert(sinksIterator.key());
                    }
                }
            }
        }

        if (pause) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Pausing all the playing media";
            this->updatePlayersList();
            QHashIterator<QString, GlobalSystemMediaTransportControlsSession> playersIterator(playersList);
            while (playersIterator.hasNext()) {
                playersIterator.next();
                auto &player = playersIterator.value();
                auto &playerName = playersIterator.key();

                auto playbackInfo = player.GetPlaybackInfo();
                auto playbackControls = playbackInfo.Controls();
                if (playbackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) {
                    if (playbackControls.IsPauseEnabled()) {
                        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Trying to pause " << playerName;
                        if (player.TryPauseAsync().get()) {
                            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Paused " << playerName;
                            pausedSources.insert(playerName);
                        }
                    } else {
                        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Pause not supported by the app! Trying to stop " << playerName;
                        if (player.TryStopAsync().get()) {
                            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Stopped " << playerName;
                            pausedSources.insert(playerName);
                        }
                    }
                }
            }
        }
    } else if (autoResume) {
        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unmuting sinks";
            QHashIterator<QString, IAudioEndpointVolume *> sinksIterator(sinksList);
            while (sinksIterator.hasNext()) {
                sinksIterator.next();
                if (mutedSinks.contains(sinksIterator.key())) {
                    qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Trying to unmute " << sinksIterator.key();
                    if (sinksIterator.value()->SetMute(false, NULL) == S_OK) {
                        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unmuted " << sinksIterator.key();
                    }
                    mutedSinks.remove(sinksIterator.key());
                }
            }
        }
        if (pause) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unpausing media";
            QHashIterator<QString, GlobalSystemMediaTransportControlsSession> playersIterator(playersList);
            while (playersIterator.hasNext()) {
                playersIterator.next();
                auto &player = playersIterator.value();
                auto &playerName = playersIterator.key();

                auto playbackInfo = player.GetPlaybackInfo();
                auto playbackControls = playbackInfo.Controls();
                if (pausedSources.contains({playerName})) {
                    if (playbackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused) {
                        qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Trying to resume " << playerName;
                        if (player.TryPlayAsync().get()) {
                            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Resumed " << playerName;
                        }
                    }
                    pausedSources.remove(playerName);
                }
            }
        }
    }
}

#include "moc_pausemusicplugin-win.cpp"
#include "pausemusicplugin-win.moc"
