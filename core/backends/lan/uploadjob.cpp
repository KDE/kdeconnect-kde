/*
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "uploadjob.h"

#include <KLocalizedString>

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"
#include <daemon.h>

UploadJob::UploadJob(const NetworkPacket &networkPacket)
    : KJob()
    , m_networkPacket(networkPacket)
    , m_input(networkPacket.payload())
    , m_socket(nullptr)
{
}

void UploadJob::setSocket(QSslSocket *socket)
{
    m_socket = socket;
    m_socket->setParent(this);
}

void UploadJob::start()
{
    if (!m_input->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; // TODO: Handle error, clean up...
    }

    if (!m_socket) {
        qCWarning(KDECONNECT_CORE) << "you must call setSocket() before calling start()";
        return;
    }

    connect(m_input.data(), &QIODevice::aboutToClose, this, &UploadJob::aboutToClose);

    bytesUploaded = 0;
    setProcessedAmount(Bytes, bytesUploaded);

    connect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);

    uploadNextPacket();
}

void UploadJob::uploadNextPacket()
{
    qint64 bytesAvailable = m_input->bytesAvailable();

    if (bytesAvailable > 0) {
        qint64 bytesToSend = qMin(m_input->bytesAvailable(), (qint64)4096);
        bytesUploading = m_socket->write(m_input->read(bytesToSend));
    }

    if (bytesAvailable <= 0 || bytesUploading < 0) {
        m_input->close();
        disconnect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);
    }
}

void UploadJob::encryptedBytesWritten(qint64 /*bytes*/)
{
    if (m_socket->encryptedBytesToWrite() == 0) {
        bytesUploaded += bytesUploading;
        setProcessedAmount(Bytes, bytesUploaded);

        uploadNextPacket();
    }
}

void UploadJob::aboutToClose()
{
    m_socket->disconnectFromHost();
    emitResult();
}

bool UploadJob::stop()
{
    m_input->close();

    return true;
}

const NetworkPacket UploadJob::getNetworkPacket()
{
    return m_networkPacket;
}

#include "moc_uploadjob.cpp"
