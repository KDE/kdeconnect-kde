/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PAUSEMUSICPLUGINWIN_H
#define PAUSEMUSICPLUGINWIN_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QHash>

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>

#include <core/kdeconnectplugin.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::ApplicationModel;

class PauseMusicPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit PauseMusicPlugin(QObject* parent, const QVariantList& args);
    ~PauseMusicPlugin();

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override { }

private:
    void updatePlayersList();
    bool updateSinksList();

    bool valid;
    IMMDeviceEnumerator *deviceEnumerator;
    QHash<QString, IAudioEndpointVolume *> sinksList;

    std::optional<GlobalSystemMediaTransportControlsSessionManager> sessionManager;
    QHash<QString, GlobalSystemMediaTransportControlsSession> playersList;

    QSet<QString> pausedSources;
    QSet<QString> mutedSinks;
};

#endif // PAUSEMUSICPLUGINWIN_H
