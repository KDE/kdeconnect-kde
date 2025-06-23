/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardplugin.h"

#include "clipboardlistener.h"
#include "core/daemon.h"
#include "core/filetransferjob.h"
#include "plugin_clipboard_debug.h"

#include <KPluginFactory>
#include <QBuffer>
#include <QEventLoop>
#include <QImage>
#include <QMimeType>
#include <QObject>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariant>
#include <qloggingcategory.h>
#include <qstringliteral.h>
#include <qtypes.h>

static constexpr qsizetype ONE_MB = 1 * 1024u * 1024u;

K_PLUGIN_CLASS_WITH_JSON(ClipboardPlugin, "kdeconnect_clipboard.json")

ClipboardPlugin::ClipboardPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    connect(ClipboardListener::instance(), &ClipboardListener::clipboardChanged, this, &ClipboardPlugin::clipboardChanged);
    connect(config(), &KdeConnectPluginConfig::configChanged, this, &ClipboardPlugin::configChanged);
    configChanged();
}

void ClipboardPlugin::connected()
{
    sendConnectPacket();
}

QString ClipboardPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/clipboard").arg(device()->id());
}

void ClipboardPlugin::clipboardChanged(const QVariant &content, ClipboardListener::ClipboardContentType contentType)
{
    if (!autoShare) {
        return;
    }

    if (contentType == ClipboardListener::ClipboardContentTypePassword) {
        if (!sharePasswords) {
            return;
        }
    }

    sendClipboard(content);
}

void ClipboardPlugin::configChanged()
{
    autoShare = config()->getBool(QStringLiteral("autoShare"), config()->getBool(QStringLiteral("sendUnknown"), true));
    sharePasswords = config()->getBool(QStringLiteral("sendPassword"), true);
    Q_EMIT autoShareDisabledChanged(isAutoShareDisabled());
}

bool ClipboardPlugin::isAutoShareDisabled()
{
    // Return true also if autoShare is enabled but disabled for passwords
    return !autoShare || !sharePasswords;
}

void ClipboardPlugin::sendClipboard()
{
    QVariant content = ClipboardListener::instance()->currentContent();
    sendClipboard(content);
}

void ClipboardPlugin::sendClipboard(const QVariant &content)
{
    if (content.typeId() == QMetaType::QUrl) {
        NetworkPacket np{PACKET_TYPE_CLIPBOARD_FILE};

        QUrl url = content.toUrl();
        QSharedPointer<QFile> ioFile(new QFile(url.toLocalFile()));

        const qsizetype fileSize = ioFile->size();
        const qsizetype configSize = config()->getInt(QStringLiteral("maxClipboardFileSizeMB"), 50) * ONE_MB;

        qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << fileSize << " " << configSize;

        if (!ioFile->exists()) {
            qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "Could not open file " << url.toDisplayString();
            return;
        } else if (fileSize > configSize) {
            qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "File exceeds the max clipboard file size " << url.toDisplayString();
            return;
        } else {
            np.setPayload(ioFile, ioFile->size());
            np.set<QString>(QStringLiteral("filename"), url.fileName());
        }

        sendPacket(np);
    } else if (content.typeId() == QMetaType::QImage) {
        const QImage imageData = qvariant_cast<QImage>(content);
        QSharedPointer<QBuffer> buffer(new QBuffer());
        buffer->open(QIODevice::ReadWrite);

        if (imageData.save(buffer.get(), CLIPBOARD_IMAGE_FORMAT, 100)) {
            const qsizetype imageSize = buffer->size();
            const qsizetype configSize = config()->getInt(QStringLiteral("maxClipboardFileSizeMB"), 50) * ONE_MB;

            if (imageSize > configSize) {
                qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "Image exceeds the max clipboard file size";
                return;
            }

            NetworkPacket np{PACKET_TYPE_CLIPBOARD_FILE};

            np.setPayload(buffer, buffer->size());
            np.set<QString>(QStringLiteral("filename"), QString::number(QDateTime::currentMSecsSinceEpoch()) + CLIPBOARD_IMAGE_EXTENSION);

            sendPacket(np);
        } else {
            qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "Could not copy clipboard image";
        }
    } else {
        NetworkPacket np{PACKET_TYPE_CLIPBOARD};

        QString contentStr = content.toString();

        np.set(QStringLiteral("content"), contentStr);
        sendPacket(np);
    }
}

void ClipboardPlugin::sendConnectPacket()
{
    if (!autoShare || (!sharePasswords && ClipboardListener::instance()->currentContentType() == ClipboardListener::ClipboardContentTypePassword)) {
        // avoid sharing clipboard if the content is no need to share
        return;
    }

    NetworkPacket np(PACKET_TYPE_CLIPBOARD_CONNECT,
                     {{QStringLiteral("content"), ClipboardListener::instance()->currentContent()},
                      {QStringLiteral("timestamp"), ClipboardListener::instance()->updateTimestamp()}});
    sendPacket(np);
}

void ClipboardPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.type() == PACKET_TYPE_CLIPBOARD_FILE) {
        if (np.hasPayload() && np.has(QStringLiteral("filename"))) {
            QTemporaryDir tmpDir;
            tmpDir.setAutoRemove(false);

            auto filename = np.get<QString>(QStringLiteral("filename"));
            QUrl destination(tmpDir.path() + QStringLiteral("/") + filename);

            FileTransferJob *job = np.createPayloadTransferJob(destination);
            job->setProperty("destUrl", destination.toString());
            job->setProperty("immediateProgressReporting", true);

            connect(job, &KJob::result, this, [](KJob *job) -> void {
                ClipboardListener::instance()->setFile(job);
            });

            job->start();
        } else {
            qCWarning(KDECONNECT_PLUGIN_CLIPBOARD) << "Ignoring file without payload or filename";
        }

        return;
    }

    QString content = np.get<QString>(QStringLiteral("content"));
    if (np.type() == PACKET_TYPE_CLIPBOARD) {
        ClipboardListener::instance()->setText(content);
    } else if (np.type() == PACKET_TYPE_CLIPBOARD_CONNECT) {
        qint64 packetTime = np.get<qint64>(QStringLiteral("timestamp"));
        // If the packetTime is 0, it means the timestamp is unknown (so do nothing).
        if (packetTime == 0 || packetTime < ClipboardListener::instance()->updateTimestamp()) {
            qCWarning(KDECONNECT_PLUGIN_CLIPBOARD) << "Ignoring clipboard without timestamp";
            return;
        }

        if (np.has(QStringLiteral("content"))) {
            ClipboardListener::instance()->setText(content);
        }
    }
}

#include "clipboardplugin.moc"
#include "moc_clipboardplugin.cpp"
