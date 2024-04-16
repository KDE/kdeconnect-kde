/*
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2015 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FILETRANSFERJOB_H
#define FILETRANSFERJOB_H

#include <KJob>

#include <QElapsedTimer>
#include <QIODevice>
#include <QNetworkReply>
#include <QSharedPointer>
#include <QUrl>

#include "kdeconnectcore_export.h"

class NetworkPacket;
/**
 * @short It will stream a device into a url destination
 *
 * Given a QIODevice, the file transfer job will use the system's QNetworkAccessManager
 * for putting the stream into the requested location.
 */
class KDECONNECTCORE_EXPORT FileTransferJob : public KJob
{
    Q_OBJECT

public:
    /**
     * @p origin specifies the data to read from.
     * @p size specifies the expected size of the stream we're reading.
     * @p destination specifies where these contents should be stored
     */
    FileTransferJob(const NetworkPacket *np, const QUrl &destination);
    void start() override;
    QUrl destination() const
    {
        return m_destination;
    }
    void setOriginName(const QString &from)
    {
        m_from = from;
    }
    void setAutoRenameIfDestinatinonExists(bool autoRename)
    {
        m_autoRename = autoRename;
    }
    const NetworkPacket *networkPacket()
    {
        return m_np;
    }

private Q_SLOTS:
    void doStart();

protected:
    bool doKill() override;

private:
    void startTransfer();
    void transferFinished();
    void deleteDestinationFile();

    QSharedPointer<QIODevice> m_origin;
    QNetworkReply *m_reply;
    QString m_from;
    QUrl m_destination;
    QElapsedTimer m_timer;
    qint64 m_written;
    qint64 m_size;
    const NetworkPacket *m_np;
    bool m_autoRename;
};

#endif
