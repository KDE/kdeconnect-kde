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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QString>

class MprisRemotePlayer {

public:
    explicit MprisRemotePlayer();
    virtual ~MprisRemotePlayer();

    void parseNetworkPacket(const NetworkPacket& np);
    long position() const;
    void setPosition(long position);
    int volume() const;
    long length() const;
    bool playing() const;
    QString nowPlaying() const;
    QString title() const;
    QString artist() const;
    QString album() const;

private:

    QString id;
    bool m_playing;
    QString m_nowPlaying;
    int m_volume;
    long m_length;
    long m_lastPosition;
    qint64 m_lastPositionTime;
    QString m_title;
    QString m_artist;
    QString m_album;
};
