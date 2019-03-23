/**
 * Copyright 2018 Nicolas Fella <nicolas.fella@gmx.de>
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

#ifndef REMOTESYSTEMVOLUMEPLUGIN_H
#define REMOTESYSTEMVOLUMEPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_SYSTEMVOLUME QStringLiteral("kdeconnect.systemvolume")
#define PACKET_TYPE_SYSTEMVOLUME_REQUEST QStringLiteral("kdeconnect.systemvolume.request")

class Q_DECL_EXPORT RemoteSystemVolumePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotesystemvolume")
    Q_PROPERTY(QByteArray sinks READ sinks NOTIFY sinksChanged)
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)

public:
    explicit RemoteSystemVolumePlugin(QObject* parent, const QVariantList& args);
    ~RemoteSystemVolumePlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;
    QString dbusPath() const override;

    QString deviceId() const { return device()->id(); }
    QByteArray sinks();

    Q_SCRIPTABLE void sendVolume(const QString& name, int volume);
    Q_SCRIPTABLE void sendMuted(const QString& name, bool muted);

Q_SIGNALS:
    Q_SCRIPTABLE void sinksChanged();
    Q_SCRIPTABLE void volumeChanged(const QString& name, int volume);
    Q_SCRIPTABLE void mutedChanged(const QString& name, bool muted);

private:
    QByteArray m_sinks;
};

#endif // REMOTESYSTEMVOLUMEPLUGIN_H
