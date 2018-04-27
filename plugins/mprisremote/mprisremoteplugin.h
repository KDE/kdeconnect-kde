/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MPRISREMOTEPLUGIN_H
#define MPRISREMOTEPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#include "mprisremoteplayer.h"

#define PACKET_TYPE_MPRIS_REQUEST QStringLiteral("kdeconnect.mpris.request")
#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

class Q_DECL_EXPORT MprisRemotePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.mprisremote")
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY propertiesChanged)
    Q_PROPERTY(int length READ length NOTIFY propertiesChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY propertiesChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY propertiesChanged)
    Q_PROPERTY(QStringList playerList READ playerList NOTIFY propertiesChanged)
    Q_PROPERTY(QString player READ player WRITE setPlayer)
    Q_PROPERTY(QString nowPlaying READ nowPlaying NOTIFY propertiesChanged)
    Q_PROPERTY(QString title READ title NOTIFY propertiesChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY propertiesChanged)
    Q_PROPERTY(QString album READ album NOTIFY propertiesChanged)

public:
    explicit MprisRemotePlugin(QObject* parent, const QVariantList &args);
    ~MprisRemotePlugin() override;

    long position() const;
    int volume() const;
    int length() const;
    bool isPlaying() const;
    QStringList playerList() const;
    QString player() const;
    QString nowPlaying() const;
    QString title() const;
    QString artist() const;
    QString album() const;

    void setVolume(int volume);
    void setPosition(int position);
    void setPlayer(const QString& player);

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}
    QString dbusPath() const override;

    Q_SCRIPTABLE void seek(int offset) const;
    Q_SCRIPTABLE void requestPlayerList();
    Q_SCRIPTABLE void sendAction(const QString& action);

Q_SIGNALS:
    void propertiesChanged();

private:
    void requestPlayerStatus(const QString& player);

    QString m_currentPlayer;
    QMap<QString, MprisRemotePlayer*> m_players;
};

#endif
