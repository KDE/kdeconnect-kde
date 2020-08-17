/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pausemusicplugin-win.h"

#include <KPluginFactory>
#include "plugin_pausemusic_debug.h"

K_PLUGIN_CLASS_WITH_JSON(PauseMusicPlugin, "kdeconnect_pausemusic.json")

PauseMusicPlugin::PauseMusicPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    CoInitialize(NULL);
    deviceEnumerator = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    defaultDevice = NULL;
    g_guidMyContext = GUID_NULL;

    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    endpointVolume = NULL;
    defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
    defaultDevice->Release();
    defaultDevice = NULL;
    CoCreateGuid(&g_guidMyContext);
}

PauseMusicPlugin::~PauseMusicPlugin()
{
    endpointVolume->Release();
    CoUninitialize();
}

bool PauseMusicPlugin::receivePacket(const NetworkPacket& np)
{

    bool pauseOnlyWhenTalking = config()->get(QStringLiteral("conditionTalking"), false);

    if (pauseOnlyWhenTalking) {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return true;
        }
    } else {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("ringing")
            && np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return true;
        }
    }

    bool pauseConditionFulfilled = !np.get<bool>(QStringLiteral("isCancel"));

    bool pause = config()->get(QStringLiteral("actionPause"), false);
    bool mute = config()->get(QStringLiteral("actionMute"), true);

    const bool autoResume = config()->get(QStringLiteral("actionResume"), true);

    if (pauseConditionFulfilled) {

        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Muting music";
            endpointVolume->SetMute(TRUE, &g_guidMyContext);
        }

        if (pause) {
            //  TODO PAUSING
        }

    } else {

        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unmuting system volume";
            if (autoResume) {
                endpointVolume->SetMute(FALSE, &g_guidMyContext);
            }
        }
        if (pause) {
            // TODO UNPAUSING
        }
    }

    return true;

}

#include "pausemusicplugin-win.moc"
