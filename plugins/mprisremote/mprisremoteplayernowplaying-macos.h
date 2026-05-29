/**
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <QObject>

class MprisRemotePlayer;
class MprisRemotePlugin;

/**
 * macOS counterpart of the MPRIS2 adaptors: publishes a remote player to the
 * system Now Playing UI and routes media key commands back to it.
 * macOS has a single Now Playing slot, owned by the most recently active player.
 */
class MprisRemotePlayerNowPlaying : public QObject
{
    Q_OBJECT

public:
    explicit MprisRemotePlayerNowPlaying(MprisRemotePlayer *parent, MprisRemotePlugin *plugin);
    ~MprisRemotePlayerNowPlaying() override;

    void sendAction(const QString &action);
    void setPosition(qint64 positionMs);

private:
    void onChanged();
    void update();

    MprisRemotePlayer *m_parent;
    MprisRemotePlugin *m_plugin;
};
