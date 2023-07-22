/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PHOTOPLUGIN_H
#define PHOTOPLUGIN_H

class QObject;

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_PHOTO_REQUEST QStringLiteral("kdeconnect.photo.request")
#define PACKET_TYPE_PHOTO QStringLiteral("kdeconnect.photo")

class Q_DECL_EXPORT PhotoPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.photo")

public:
    explicit PhotoPlugin(QObject *parent, const QVariantList &args);

    Q_SCRIPTABLE void requestPhoto(const QString &url);

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override
    {
    }

    QString dbusPath() const override;

Q_SIGNALS:
    Q_SCRIPTABLE void photoReceived(const QString &url);

private:
    QStringList requestedFiles;
};

#endif
