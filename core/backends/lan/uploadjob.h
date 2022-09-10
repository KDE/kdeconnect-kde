/*
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef UPLOADJOB_H
#define UPLOADJOB_H

#include <KJob>

#include "server.h"
#include <QElapsedTimer>
#include <QIODevice>
#include <QSslSocket>
#include <QVariantMap>
#include <networkpacket.h>

class KDECONNECTCORE_EXPORT UploadJob : public KJob
{
    Q_OBJECT
public:
    explicit UploadJob(const NetworkPacket &networkPacket);

    void setSocket(QSslSocket *socket);
    void start() override;
    bool stop();
    const NetworkPacket getNetworkPacket();

private:
    const NetworkPacket m_networkPacket;
    QSharedPointer<QIODevice> m_input;
    QSslSocket *m_socket;
    qint64 bytesUploading;
    qint64 bytesUploaded;

    const static quint16 MIN_PORT = 1739;
    const static quint16 MAX_PORT = 1764;

private Q_SLOTS:
    void uploadNextPacket();
    void encryptedBytesWritten(qint64 bytes);
    void aboutToClose();
};

#endif // UPLOADJOB_H
