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

#ifndef FILETRANSFERJOB_H
#define FILETRANSFERJOB_H

#include <QIODevice>
#include <QTime>
#include <QTemporaryFile>
#include <QSharedPointer>

#include <KJob>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QBuffer>

#include "kdeconnectcore_export.h"

/**
 * @short It will stream a device into a url destination
 *
 * Given a QIODevice, the file transfer job will use the system's QNetworkAccessManager
 * for putting the stream into the requested location.
 */
class KDECONNECTCORE_EXPORT FileTransferJob
    : public KJob
{
    Q_OBJECT

public:
    /**
     * @p origin specifies the data to read from.
     * @p size specifies the expected size of the stream we're reading.
     * @p destination specifies where these contents should be stored
     */
    FileTransferJob(const QSharedPointer<QIODevice>& origin, qint64 size, const QUrl &destination);
    virtual void start() Q_DECL_OVERRIDE;
    QUrl destination() const { return mDestination; }
    void setOriginName(QString from) { mFrom = from; }

private Q_SLOTS:
    void doStart();

protected:
    bool doKill() Q_DECL_OVERRIDE;

private:
    void startTransfer();
    void transferFinished();

    QSharedPointer<QIODevice> mOrigin;
    QNetworkReply* mReply;
    QString mFrom;
    QUrl mDestination;
    QTime mTime;
    qulonglong mSpeedBytes;
    qint64 mWritten;
};

#endif
