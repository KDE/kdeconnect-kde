/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
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

#include "pausemusicplugin-win.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(PauseMusicPlugin, "kdeconnect_pausemusic.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PAUSEMUSIC, "kdeconnect.plugin.pausemusic")

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
