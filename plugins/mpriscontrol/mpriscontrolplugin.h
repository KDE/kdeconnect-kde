/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDBusServiceWatcher>
#include <QHash>
#include <QSharedPointer>
#include <QString>

#include <core/kdeconnectplugin.h>

class OrgFreedesktopDBusPropertiesInterface;
class OrgMprisMediaPlayer2PlayerInterface;

class MprisPlayer
{
public:
    MprisPlayer(const QString &serviceName, const QString &dbusObjectPath, const QDBusConnection &busConnection);
    MprisPlayer() = delete;

public:
    const QString &serviceName() const
    {
        return m_serviceName;
    }
    OrgFreedesktopDBusPropertiesInterface *propertiesInterface() const
    {
        return m_propertiesInterface.data();
    }
    OrgMprisMediaPlayer2PlayerInterface *mediaPlayer2PlayerInterface() const
    {
        return m_mediaPlayer2PlayerInterface.data();
    }

private:
    QString m_serviceName;
    QSharedPointer<OrgFreedesktopDBusPropertiesInterface> m_propertiesInterface;
    QSharedPointer<OrgMprisMediaPlayer2PlayerInterface> m_mediaPlayer2PlayerInterface;
};

#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

class MprisControlPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MprisControlPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;

private Q_SLOTS:
    void propertiesChanged(const QString &propertyInterface, const QVariantMap &properties);
    void seeked(qlonglong);

private:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void addPlayer(const QString &serviceName);
    void removePlayer(const QString &serviceName);
    void sendPlayerList();
    void mprisPlayerMetadataToNetworkPacket(NetworkPacket &np, const QVariantMap &nowPlayingMap) const;
    bool sendAlbumArt(const NetworkPacket &np);

    QHash<QString, MprisPlayer> playerList;
    int prevVolume;
    QDBusServiceWatcher *m_watcher;
};
