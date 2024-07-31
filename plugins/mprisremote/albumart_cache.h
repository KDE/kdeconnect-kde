/**
 * SPDX-FileCopyrightText: 2023 Krut Patel <kroot.patel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ALBUMARTCACHE_H
#define ALBUMARTCACHE_H

#include "networkpacket.h"
#include <QCache>
#include <QDir>
#include <QObject>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <optional>

class MprisRemotePlugin;

/**
 * Wrapper that automatically deletes the file from disk when destroyed.
 */
class LocalFile
{
public:
    QUrl localPath;
    explicit LocalFile(QUrl localPath)
        : localPath(std::move(localPath))
    {
    }

    ~LocalFile()
    {
        // delete the file from disk
        // TODO: Log warning if this failed
        QFile::remove(localPath.toLocalFile());
    }
};

class AlbumArtCache : public QObject
{
    Q_OBJECT

    AlbumArtCache();

    ~AlbumArtCache() override;

public:
    struct IndexItem {
        enum Status {
            FETCHING,
            SUCCESS,
            FAILED,
        };
        Status fetchStatus;
        QSharedPointer<LocalFile> file;

        explicit IndexItem(QUrl localPath)
            : fetchStatus(Status::SUCCESS)
            , file(new LocalFile{std::move(localPath)})
        {
        }
        explicit IndexItem(Status fetchStatus)
            : fetchStatus(fetchStatus)
            , file(nullptr)
        {
            // Need localPath in this case
            Q_ASSERT(fetchStatus != Status::SUCCESS);
        }
    };

    static AlbumArtCache *instance();

    /**
     * @brief Get the Album Art object. Called from mpris media player to be sent to dbus.
     *
     * @return IndexItem Current status of the art file.
     */
    static IndexItem getAlbumArt(const QString &remoteUrl, MprisRemotePlugin *plugin, const QString &player);

    /**
     * @brief Callback for when plugin receives a packet with album art.
     * Can't use slot since NetworkPacket isn't a registered type.
     */
    void handleAlbumArt(const NetworkPacket &np);

    // called by handleAlbumArt when the album art is fetched and stored to disk
Q_SIGNALS:

    void albumArtFetched(const QString &player, const QString &remoteUrl, QSharedPointer<LocalFile> localPath);

private:
    using Index = QCache<QString, IndexItem>;
    QDir m_cacheDir;
    Index m_cachedFiles;
    QReadWriteLock m_cacheLock;
};

#endif // ALBUMARTCACHE_H
