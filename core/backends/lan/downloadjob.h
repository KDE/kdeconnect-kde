/*
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef DOWNLOADJOB_H
#define DOWNLOADJOB_H

#include <KJob>

#include <QIODevice>
#include <QVariantMap>
#include <QHostAddress>
#include <QSharedPointer>
#include <QSslSocket>
#include <QBuffer>

#include "kdeconnectcore_export.h"


class KDECONNECTCORE_EXPORT DownloadJob
    : public KJob
{
    Q_OBJECT
public:
    DownloadJob(const QHostAddress& address, const QVariantMap& transferInfo);
    ~DownloadJob() override;
    void start() override;
    QSharedPointer<QIODevice> getPayload();

private:
    QHostAddress m_address;
    qint16 m_port;
    QSharedPointer<QSslSocket> m_socket;

private Q_SLOTS:
    void socketFailed(QAbstractSocket::SocketError error);
    void socketConnected();
};

#endif // UPLOADJOB_H
