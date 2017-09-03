/*
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2015 Aleix Pol i Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filetransferjob.h"
#include "daemon.h"
#include <core_debug.h>

#include <qalgorithms.h>
#include <QFileInfo>
#include <QDebug>

#include <KLocalizedString>

FileTransferJob::FileTransferJob(const QSharedPointer<QIODevice>& origin, qint64 size, const QUrl& destination)
    : KJob()
    , m_origin(origin)
    , m_reply(Q_NULLPTR)
    , m_from(QStringLiteral("KDE Connect"))
    , m_destination(destination)
    , m_speedBytes(0)
    , m_written(0)
    , m_size(size)
{
    Q_ASSERT(m_origin);
    Q_ASSERT(m_origin->isReadable());
    if (m_destination.scheme().isEmpty()) {
        qCWarning(KDECONNECT_CORE) << "Destination QUrl" << m_destination << "lacks a scheme. Setting its scheme to 'file'.";
        m_destination.setScheme(QStringLiteral("file"));
    }

    setCapabilities(Killable);
    qCDebug(KDECONNECT_CORE) << "FileTransferJob Downloading payload to" << destination << "size:" << size;
}

void FileTransferJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
    //qCDebug(KDECONNECT_CORE) << "FileTransferJob start";
}

void FileTransferJob::doStart()
{
    description(this, i18n("Receiving file over KDE Connect"),
        { i18nc("File transfer origin", "From"), m_from }
    );

    if (m_destination.isLocalFile() && QFile::exists(m_destination.toLocalFile())) {
        setError(2);
        setErrorText(i18n("Filename already present"));
        emitResult();
        return;
    }

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
    description(this, i18n("Receiving file over KDE Connect"),
                        { i18nc("File transfer origin", "From"), m_from },
                        { i18nc("File transfer destination", "To"), m_destination.toLocalFile() });

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
    });
    connect(m_reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &FileTransferJob::transferFailed);
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
    //TODO: MD5-check the file
    qCDebug(KDECONNECT_CORE) << "Finished transfer" << m_destination;

    emitResult();
}

bool FileTransferJob::doKill()
{
    if (m_reply) {
        m_reply->close();
    }
    if (m_origin) {
        m_origin->close();
    }
    return true;
}
