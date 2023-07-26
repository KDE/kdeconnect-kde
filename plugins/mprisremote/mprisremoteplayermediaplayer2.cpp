/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mprisremoteplayermediaplayer2.h"
#include "mprisremoteplayer.h"
#include "mprisremoteplugin.h"

MprisRemotePlayerMediaPlayer2::MprisRemotePlayerMediaPlayer2(MprisRemotePlayer *parent, const MprisRemotePlugin *plugin)
    : QDBusAbstractAdaptor{parent}
    , m_parent{parent}
    , m_plugin{plugin}
{
}

bool MprisRemotePlayerMediaPlayer2::CanQuit() const
{
    return false;
}

bool MprisRemotePlayerMediaPlayer2::CanRaise() const
{
    return false;
}

bool MprisRemotePlayerMediaPlayer2::HasTrackList() const
{
    return false;
}

QString MprisRemotePlayerMediaPlayer2::DesktopEntry() const
{
    // Allows controlling mpris from the KDE Connect application's taskbar entry.
    return QStringLiteral("org.kde.kdeconnect.app");
}

QString MprisRemotePlayerMediaPlayer2::Identity() const
{
    return m_parent->identity() + QStringLiteral(" - ") + m_plugin->device()->name();
}

void MprisRemotePlayerMediaPlayer2::Quit()
{
}

void MprisRemotePlayerMediaPlayer2::Raise()
{
}

QStringList MprisRemotePlayerMediaPlayer2::SupportedUriSchemes() const
{
    return {};
}

QStringList MprisRemotePlayerMediaPlayer2::SupportedMimeTypes() const
{
    return {};
}

#include "moc_mprisremoteplayermediaplayer2.cpp"
