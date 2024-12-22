/**
 * SPDX-FileCopyrightText: 2017 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "systemvolumeplugin-pulse.h"

#include <KPluginFactory>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <core/device.h>

#include "plugin_systemvolume_debug.h"
#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>

K_PLUGIN_CLASS_WITH_JSON(SystemvolumePlugin, "kdeconnect_systemvolume.json")

SystemvolumePlugin::SystemvolumePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , sinksMap()
{
}

void SystemvolumePlugin::receivePacket(const NetworkPacket &np)
{
    if (!PulseAudioQt::Context::instance()->isValid())
        return;

    if (np.has(QStringLiteral("requestSinks"))) {
        sendSinkList();
    } else {
        QString name = np.get<QString>(QStringLiteral("name"));

        PulseAudioQt::Sink *sink = sinksMap.value(name);
        if (sink) {
            if (np.has(QStringLiteral("volume"))) {
                int volume = np.get<int>(QStringLiteral("volume"));
                sink->setVolume(volume);
                sink->setMuted(false);
            }
            if (np.has(QStringLiteral("muted"))) {
                sink->setMuted(np.get<bool>(QStringLiteral("muted")));
            }
            if (np.has(QStringLiteral("enabled"))) {
                sink->setDefault(np.get<bool>(QStringLiteral("enabled")));
            }
        }
    }
}

void SystemvolumePlugin::sendSinkList()
{
    QJsonDocument document;
    QJsonArray array;

    sinksMap.clear();

    const auto sinks = PulseAudioQt::Context::instance()->sinks();
    for (PulseAudioQt::Sink *sink : sinks) {
        sinksMap.insert(sink->name(), sink);

        connect(sink, &PulseAudioQt::Sink::volumeChanged, this, [this, sink] {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<int>(QStringLiteral("volume"), sink->volume());
            np.set<QString>(QStringLiteral("name"), sink->name());
            sendPacket(np);
        });

        connect(sink, &PulseAudioQt::Sink::mutedChanged, this, [this, sink] {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<bool>(QStringLiteral("muted"), sink->isMuted());
            np.set<QString>(QStringLiteral("name"), sink->name());
            sendPacket(np);
        });

        connect(sink, &PulseAudioQt::Sink::defaultChanged, this, [this, sink] {
            NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
            np.set<bool>(QStringLiteral("enabled"), sink->isDefault());
            np.set<QString>(QStringLiteral("name"), sink->name());
            sendPacket(np);
        });

        QJsonObject sinkObject{{QStringLiteral("name"), sink->name()},
                               {QStringLiteral("muted"), sink->isMuted()},
                               {QStringLiteral("description"), sink->description()},
                               {QStringLiteral("volume"), sink->volume()},
                               {QStringLiteral("maxVolume"), PulseAudioQt::normalVolume()},
                               {QStringLiteral("enabled"), sink->isDefault()}};

        array.append(sinkObject);
    }

    document.setArray(array);

    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME);
    np.set<QJsonDocument>(QStringLiteral("sinkList"), document);
    sendPacket(np);
}

void SystemvolumePlugin::connected()
{
    connect(PulseAudioQt::Context::instance(), &PulseAudioQt::Context::sinkAdded, this, [this] {
        sendSinkList();
    });

    connect(PulseAudioQt::Context::instance(), &PulseAudioQt::Context::sinkRemoved, this, [this] {
        sendSinkList();
    });

    // Send the initial sink list. Despite the name, this function also recreates the sink map
    sendSinkList();
}

#include "moc_systemvolumeplugin-pulse.cpp"
#include "systemvolumeplugin-pulse.moc"
