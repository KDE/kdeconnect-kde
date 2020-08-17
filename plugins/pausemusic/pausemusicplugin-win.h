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

#include <core/kdeconnectplugin.h>

#include <mmdeviceapi.h>
#include <endpointvolume.h>

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
    IMMDeviceEnumerator *deviceEnumerator;
    IMMDevice *defaultDevice;
    GUID g_guidMyContext;
    IAudioEndpointVolume *endpointVolume;

};

#endif // PAUSEMUSICPLUGINWIN_H
