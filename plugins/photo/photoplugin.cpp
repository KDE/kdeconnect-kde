/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "photoplugin.h"

#include <KPluginFactory>

#include <QDebug>

#include "plugin_photo_debug.h"
#include <core/filetransferjob.h>

K_PLUGIN_CLASS_WITH_JSON(PhotoPlugin, "kdeconnect_photo.json")

void PhotoPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<bool>(QStringLiteral("cancel"))) {
        requestedFiles.takeFirst();
    }

    if (requestedFiles.isEmpty() || !np.hasPayload()) {
        return;
    }

    const QString url = requestedFiles.takeFirst();
    FileTransferJob *job = np.createPayloadTransferJob(QUrl(url));
    connect(job, &FileTransferJob::result, this, [this, url] {
        Q_EMIT photoReceived(url);
    });
    job->start();
}

void PhotoPlugin::requestPhoto(const QString &url)
{
    requestedFiles.append(url);
    NetworkPacket np(PACKET_TYPE_PHOTO_REQUEST);
    sendPacket(np);
}

QString PhotoPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/photo").arg(device()->id());
}

#include "moc_photoplugin.cpp"
#include "photoplugin.moc"
