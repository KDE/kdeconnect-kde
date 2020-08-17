/**
 * SPDX-FileCopyrightText: 2017 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SYSTEMVOLUMEPLUGINPULSE_H
#define SYSTEMVOLUMEPLUGINPULSE_H

#include <QObject>
#include <QMap>

#include <core/kdeconnectplugin.h>

#include <PulseAudioQt/Sink>

#define PACKET_TYPE_SYSTEMVOLUME QStringLiteral("kdeconnect.systemvolume")
#define PACKET_TYPE_SYSTEMVOLUME_REQUEST QStringLiteral("kdeconnect.systemvolume.request")


class Q_DECL_EXPORT SystemvolumePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit SystemvolumePlugin(QObject* parent, const QVariantList& args);

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

private:
    void sendSinkList();
    QMap<QString, PulseAudioQt::Sink*> sinksMap;
};

#endif
