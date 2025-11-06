
#include "albumart_cache.h"

#include <QDir>
#include <QReadLocker>
#include <QStandardPaths>

#include "filetransferjob.h"
#include "kjob.h"
#include "mprisremoteplugin.h"
#include "plugin_mprisremote_debug.h"

// TODO: Not sure where to put such utils
constexpr std::size_t operator""_MB(unsigned long long v)
{
    return 1024u * 1024u * v;
}

static constexpr qsizetype CACHE_SIZE = 5_MB;

AlbumArtCache::AlbumArtCache()
{
    m_cachedFiles.setMaxCost(CACHE_SIZE);
    m_cacheDir = QDir{QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append(QStringLiteral("/kdeconnect/albumart"))};
    if (!m_cacheDir.exists()) {
        m_cacheDir.mkpath(QStringLiteral("."));
    } else {
        // clear the directory
        // TODO: Better thing to do would be to re-populate the m_cachedFiles
        qDebug() << "Clearing existing entries" << m_cacheDir.entryList(QDir::Files).size();
        for (auto &file : m_cacheDir.entryList(QDir::Files)) {
            m_cacheDir.remove(file);
        }
    }
}

AlbumArtCache::~AlbumArtCache() = default;

AlbumArtCache *AlbumArtCache::instance()
{
    static auto *s_albumArtCache = new AlbumArtCache();

    return s_albumArtCache;
}

AlbumArtCache::IndexItem AlbumArtCache::getAlbumArt(const QString &remoteUrl, MprisRemotePlugin *plugin, const QString &player)
{
    if (remoteUrl.isEmpty()) {
        // Can't fetch an empty remoteUrl. Do we want to add a separate status for this?
        return IndexItem{IndexItem::FAILED};
    }
    QReadLocker locker{&instance()->m_cacheLock};
    IndexItem *item = instance()->m_cachedFiles.object(remoteUrl);
    if (item != nullptr) {
        if (item->fetchStatus == IndexItem::SUCCESS) {
            qCDebug(KDECONNECT_PLUGIN_MPRISREMOTE) << "album art already present" << remoteUrl << "at" << item->file->localPath;
            if (!item->file->localPath.isLocalFile()) {
                qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "No file for cached art!" << item->file->localPath;
                return IndexItem{IndexItem::FAILED};
            }
        }
        return *item;
    } else {
        // TODO: First check if we are already fetching it

        // fetch the album art from plugin
        plugin->requestAlbumArt(player, remoteUrl);
        return IndexItem{IndexItem::FETCHING};
    }
}

void AlbumArtCache::handleAlbumArt(const NetworkPacket &np)
{
    if (!np.hasPayload()) {
        qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "Empty album art! Ignoring.";
        return;
    }
    if (np.payloadSize() > CACHE_SIZE) {
        qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "Art is too big! Ignoring.";
        return;
    }
    auto remoteUrl = np.get<QString>(QStringLiteral("albumArtUrl"));
    if (remoteUrl.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "No url with art";
        return;
    }
    auto player = np.get<QString>(QStringLiteral("player"));
    // TODO: Track in-flight requests and reject if not present
    {
        QReadLocker locker{&instance()->m_cacheLock};
        auto *entry = m_cachedFiles.object(remoteUrl);
        if (entry != nullptr) {
            if (entry->fetchStatus == IndexItem::SUCCESS) {
                qCDebug(KDECONNECT_PLUGIN_MPRISREMOTE) << "Cache hit" << entry->file->localPath.fileName() << "for" << remoteUrl;
                Q_EMIT albumArtFetched(player, remoteUrl, entry->file);
                return;
            } else {
                // fetch again
            }
        }
    }
    // FIXME: better local file path
    auto filename = QStringLiteral("%1.jpg").arg(qHash(remoteUrl));
    auto localUrl = QUrl::fromLocalFile(m_cacheDir.filePath(filename));
    auto *job = np.createPayloadTransferJob(localUrl);
    connect(job, &FileTransferJob::result, this, [this, job, remoteUrl, localUrl, player]() {
        if (job->error()) {
            // TODO: Handle error, retry?
            qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "art transfer error" << remoteUrl << "to" << localUrl << job->errorString();
            return;
        }
        auto fileSize = static_cast<qsizetype>(job->totalAmount(KJob::Unit::Bytes));
        qCInfo(KDECONNECT_PLUGIN_MPRISREMOTE) << "Finished art transfer! from" << remoteUrl << "to" << localUrl << "size" << fileSize;
        auto *indexItem = new IndexItem{localUrl};
        auto localPath = indexItem->file;
        QWriteLocker locker{&instance()->m_cacheLock};
        m_cachedFiles.insert(remoteUrl, indexItem, fileSize);
        if (!QFile{localUrl.toLocalFile()}.exists()) {
            qCWarning(KDECONNECT_PLUGIN_MPRISREMOTE) << "File doesn't exist" << localUrl;
            return;
        }
        Q_EMIT albumArtFetched(player, remoteUrl, localPath);
    });
    job->start();
}
