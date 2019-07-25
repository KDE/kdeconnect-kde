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
#pragma once

#include <QDBusAbstractAdaptor>

class MprisRemotePlayer;
class MprisRemotePlugin;

class MprisRemotePlayerMediaPlayer2 : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ CanQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ CanRaise CONSTANT)
    Q_PROPERTY(bool HasTrackList READ HasTrackList CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry CONSTANT)
    Q_PROPERTY(QString Identity READ Identity CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes CONSTANT)

public:
    explicit MprisRemotePlayerMediaPlayer2(MprisRemotePlayer *parent, const MprisRemotePlugin *plugin);
    ~MprisRemotePlayerMediaPlayer2();

public Q_SLOTS:
    void Raise();
    void Quit();

public:
    bool CanQuit() const;
    bool CanRaise() const;
    bool HasTrackList() const;
    QString DesktopEntry() const;
    QString Identity() const;
    QStringList SupportedUriSchemes() const;
    QStringList SupportedMimeTypes() const;

private:
    const MprisRemotePlayer *m_parent;
    const MprisRemotePlugin *m_plugin;
};
