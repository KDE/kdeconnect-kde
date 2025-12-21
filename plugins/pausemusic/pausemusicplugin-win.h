/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>

#include <core/kdeconnectplugin.h>

#include <endpointvolume.h>
#include <mmdeviceapi.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::ApplicationModel;

class PauseMusicPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit PauseMusicPlugin(QObject *parent, const QVariantList &args);
    ~PauseMusicPlugin() override;

    void receivePacket(const NetworkPacket &np) override;

private:
    void updatePlayersList();
    bool updateSinksList();

    bool valid;
    IMMDeviceEnumerator *deviceEnumerator;
    QHash<QString, IAudioEndpointVolume *> sinksList;

    QHash<QString, GlobalSystemMediaTransportControlsSession> playersList;

    QSet<QString> pausedSources;
    QSet<QString> mutedSinks;
};
