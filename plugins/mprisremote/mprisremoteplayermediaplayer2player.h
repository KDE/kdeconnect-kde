/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QVariantMap>

class MprisRemotePlayer;
class MprisRemotePlugin;

class MprisRemotePlayerMediaPlayer2Player : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
    Q_PROPERTY(double Rate READ Rate CONSTANT)
    Q_PROPERTY(QVariantMap Metadata READ Metadata)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ Position)
    Q_PROPERTY(double MinimumRate READ MinimumRate)
    Q_PROPERTY(double MaximumRate READ MaximumRate)
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    Q_PROPERTY(bool CanPlay READ CanPlay)
    Q_PROPERTY(bool CanPause READ CanPause)
    Q_PROPERTY(bool CanSeek READ CanSeek CONSTANT)
    Q_PROPERTY(bool CanControl READ CanControl CONSTANT)

public:
    explicit MprisRemotePlayerMediaPlayer2Player(MprisRemotePlayer *parent, MprisRemotePlugin *plugin);

public Q_SLOTS:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(QDBusObjectPath TrackId, qlonglong Position);
    void OpenUri(QString Uri);

public:
    QString PlaybackStatus() const;
    double Rate() const;
    QVariantMap Metadata() const;
    double Volume() const;
    void setVolume(double volume) const;
    qlonglong Position() const;
    double MinimumRate() const;
    double MaximumRate() const;
    bool CanGoNext() const;
    bool CanGoPrevious() const;
    bool CanPlay() const;
    bool CanPause() const;
    bool CanSeek() const;
    bool CanControl() const;

Q_SIGNALS:
    void Seeked(qlonglong position);

private:
    MprisRemotePlayer *m_parent;
    MprisRemotePlugin *m_plugin;

    bool m_controlsChanged;
    bool m_trackInfoChanged;
    bool m_positionChanged;
    bool m_volumeChanged;
    bool m_playingChanged;

    void controlsChanged();
    void trackInfoChanged();
    void positionChanged();
    void volumeChanged();
    void playingChanged();
    void emitPropertiesChanged();
};
