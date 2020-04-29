/**
 * Copyright 2017 Nicolas Fella <nicolas.fella@gmx.de>
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

#include "systemvolumeplugin-pulse.h"

#include <KPluginFactory>

#include <QDebug>
#include <QLoggingCategory>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <core/device.h>

#include <PulseAudioQt/Sink>
#include <PulseAudioQt/Context>

K_PLUGIN_CLASS_WITH_JSON(SystemvolumePlugin, "kdeconnect_systemvolume.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SYSTEMVOLUME, "kdeconnect.plugin.systemvolume")

SystemvolumePlugin::SystemvolumePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , sinksMap()
{}

bool SystemvolumePlugin::receivePacket(const NetworkPacket& np)
{

    if (!PulseAudioQt::Context::instance()->isValid())
        return false;

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
        }
    }
    return true;
}

void SystemvolumePlugin::sendSinkList() {

    QJsonDocument document;
    QJsonArray array;

    sinksMap.clear();

    const auto sinks = PulseAudioQt::Context::instance()->sinks();
    for (PulseAudioQt::Sink* sink : sinks) {
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

        QJsonObject sinkObject {
            {QStringLiteral("name"), sink->name()},
            {QStringLiteral("muted"), sink->isMuted()},
            {QStringLiteral("description"), sink->description()},
            {QStringLiteral("volume"), sink->volume()},
            {QStringLiteral("maxVolume"), PulseAudioQt::normalVolume()}
        };

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

    const auto sinks = PulseAudioQt::Context::instance()->sinks();
    for (PulseAudioQt::Sink* sink : sinks) {
        sinksMap.insert(sink->name(), sink);
    }
}

#include "systemvolumeplugin-pulse.moc"

