/**
 * SPDX-FileCopyrightText: 2014 Apoorv Parle <apparle@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FINDMYPHONEPLUGIN_H
#define FINDMYPHONEPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_FINDMYPHONE_REQUEST QStringLiteral("kdeconnect.findmyphone.request")

class FindMyPhonePlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.findmyphone")

public:
    explicit FindMyPhonePlugin(QObject *parent, const QVariantList &args);

    Q_SCRIPTABLE void ring();

    QString dbusPath() const override;
    bool receivePacket(const NetworkPacket &np) override;
};

#endif // FINDMYPHONEPLUGIN_H
