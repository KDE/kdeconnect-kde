/*
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2015 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "filetransferjob.h"
#include "daemon.h"
#include <core_debug.h>

#include <QDebug>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <qalgorithms.h>

#include <KFileUtils>
#include <KLocalizedString>

FileTransferJob::FileTransferJob(const NetworkPacket *np, const QUrl &destination)
    : KJob()
    , m_origin(np->payload())
    , m_reply(nullptr)
    , m_from(QStringLiteral("KDE Connect"))
    , m_destination(destination)
    , m_written(0)
    , m_size(np->payloadSize())
    , m_np(np)
    , m_autoRename(false)
{
    Q_ASSERT(m_origin);
    // Disabled this assert: QBluetoothSocket doesn't report "->isReadable() == true" until it's connected
    // Q_ASSERT(m_origin->isReadable());
    if (m_destination.scheme().isEmpty()) {
        qCWarning(KDECONNECT_CORE) << "Destination QUrl" << m_destination << "lacks a scheme. Setting its scheme to 'file'.";
        m_destination.setScheme(QStringLiteral("file"));
    }

    setCapabilities(Killable);
    qCDebug(KDECONNECT_CORE) << "FileTransferJob Downloading payload to" << destination << "size:" << m_size;
}

void FileTransferJob::start()
{
    if (m_destination.isLocalFile() && QFile::exists(m_destination.toLocalFile())) {
        if (m_autoRename) {
            QFileInfo fileInfo(m_destination.toLocalFile());
            QString path = fileInfo.path();
            QString fileName = fileInfo.fileName();
            m_destination.setPath(path + QStringLiteral("/") + KFileUtils::suggestName(QUrl(path), fileName), QUrl::DecodedMode);
        } else {
            setError(2);
            setErrorText(i18n("Filename already present"));
            emitResult();
            return;
        }
    }
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
    // qCDebug(KDECONNECT_CORE) << "FileTransferJob start";
}

void FileTransferJob::doStart()
{
    if (m_origin->bytesAvailable())
        startTransfer();

    connect(m_origin.data(), &QIODevice::readyRead, this, &FileTransferJob::startTransfer);
}

void FileTransferJob::startTransfer()
{
    // Don't put each ready read
    if (m_reply)
        return;

    setProcessedAmount(Bytes, 0);

    QNetworkRequest req(m_destination);
    if (m_size >= 0) {
        setTotalAmount(Bytes, m_size);
        req.setHeader(QNetworkRequest::ContentLengthHeader, m_size);
    }
    m_reply = Daemon::instance()->networkAccessManager()->put(req, m_origin.data());

    connect(m_reply, &QNetworkReply::uploadProgress, this, [this](qint64 bytesSent, qint64 /*bytesTotal*/) {
        if (!m_timer.isValid())
            m_timer.start();
        setProcessedAmount(Bytes, bytesSent);

        const auto elapsed = m_timer.elapsed();
        if (elapsed > 0) {
            emitSpeed((1000 * bytesSent) / elapsed);
        }

        m_written = bytesSent;
    });
    connect(m_reply, &QNetworkReply::errorOccurred, this, &FileTransferJob::transferFailed);
    connect(m_reply, &QNetworkReply::finished, this, &FileTransferJob::transferFinished);
}

void FileTransferJob::transferFailed(QNetworkReply::NetworkError error)
{
    qCDebug(KDECONNECT_CORE) << "Couldn't transfer the file successfully" << error << m_reply->errorString();
    setError(error);
    setErrorText(i18n("Received incomplete file: %1", m_reply->errorString()));
    emitResult();

    m_reply->close();
}

void FileTransferJob::transferFinished()
{
    // TODO: MD5-check the file
    if (m_size == m_written) {
        qCDebug(KDECONNECT_CORE) << "Finished transfer" << m_destination;
        emitResult();
    } else {
        qCDebug(KDECONNECT_CORE) << "Received incomplete file (" << m_written << "/" << m_size << "bytes ), deleting";

        deleteDestinationFile();

        setError(3);
        setErrorText(i18n("Received incomplete file from: %1", m_from));
        emitResult();
    }
}

void FileTransferJob::deleteDestinationFile()
{
    if (m_destination.isLocalFile() && QFile::exists(m_destination.toLocalFile())) {
        QFile::remove(m_destination.toLocalFile());
    }
}

bool FileTransferJob::doKill()
{
    if (m_reply) {
        m_reply->close();
    }
    if (m_origin) {
        m_origin->close();
    }

    deleteDestinationFile();

    return true;
}

#include "moc_filetransferjob.cpp"
