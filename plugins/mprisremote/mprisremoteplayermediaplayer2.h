/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <QDBusAbstractAdaptor>

class MprisRemotePlayer;
class MprisRemotePlugin;

class MprisRemotePlayerMediaPlayer2 : public QDBusAbstractAdaptor
{
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
