/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "photoplugin.h"

#include <KPluginFactory>

#include <QDebug>

#include <core/filetransferjob.h>
#include "plugin_photo_debug.h"

K_PLUGIN_CLASS_WITH_JSON(PhotoPlugin, "kdeconnect_photo.json")

PhotoPlugin::PhotoPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

PhotoPlugin::~PhotoPlugin()
{
}

bool PhotoPlugin::receivePacket(const NetworkPacket& np)
{

    if (np.get<bool>(QStringLiteral("cancel"))) {
        requestedFiles.takeFirst();
    }

    if (requestedFiles.isEmpty() || !np.hasPayload()) {
        return true;
    }

    const QString& fileName = requestedFiles.takeFirst();
    FileTransferJob* job = np.createPayloadTransferJob(QUrl::fromLocalFile(fileName));
    connect(job, &FileTransferJob::result, this, [this, fileName] {
        Q_EMIT photoReceived(fileName);
    });
    job->start();
    return true;
}

void PhotoPlugin::requestPhoto(const QString& fileName)
{
    requestedFiles.append(fileName);
    NetworkPacket np(PACKET_TYPE_PHOTO_REQUEST);
    sendPacket(np);
}

QString PhotoPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/photo");
}

#include "photoplugin.moc"

