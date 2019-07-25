/**
 * Copyright 2019 Matthijs Tijink <matthijstijink@gmail.com>
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

#include "mprisremoteplayer.h"
#include "mprisremoteplayermediaplayer2.h"
#include "mprisremoteplugin.h"

MprisRemotePlayerMediaPlayer2::MprisRemotePlayerMediaPlayer2(MprisRemotePlayer* parent, const MprisRemotePlugin *plugin) :
    QDBusAbstractAdaptor{parent}, m_parent{parent}, m_plugin{plugin} {}

MprisRemotePlayerMediaPlayer2::~MprisRemotePlayerMediaPlayer2() = default;

bool MprisRemotePlayerMediaPlayer2::CanQuit() const {
    return false;
}

bool MprisRemotePlayerMediaPlayer2::CanRaise() const {
    return false;
}

bool MprisRemotePlayerMediaPlayer2::HasTrackList() const {
    return false;
}

QString MprisRemotePlayerMediaPlayer2::DesktopEntry() const {
    //Allows controlling mpris from the KDE Connect application's taskbar entry.
    return QStringLiteral("org.kde.kdeconnect.app");
}

QString MprisRemotePlayerMediaPlayer2::Identity() const {
    return m_parent->identity() + QStringLiteral(" - ") + m_plugin->device()->name();
}

void MprisRemotePlayerMediaPlayer2::Quit() {}

void MprisRemotePlayerMediaPlayer2::Raise() {}

QStringList MprisRemotePlayerMediaPlayer2::SupportedUriSchemes() const {
    return {};
}

QStringList MprisRemotePlayerMediaPlayer2::SupportedMimeTypes() const {
    return {};
}
