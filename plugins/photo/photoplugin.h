/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PHOTOPLUGIN_H
#define PHOTOPLUGIN_H

class QObject;

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_PHOTO_REQUEST QStringLiteral("kdeconnect.photo.request")
#define PACKET_TYPE_PHOTO QStringLiteral("kdeconnect.photo")

class Q_DECL_EXPORT PhotoPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.photo")

public:
    explicit PhotoPlugin(QObject* parent, const QVariantList& args);
    ~PhotoPlugin() override;

    Q_SCRIPTABLE void requestPhoto(const QString& fileName);

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}

    QString dbusPath() const override;

Q_SIGNALS:
    Q_SCRIPTABLE void photoReceived(const QString& fileName);

private:
    QStringList requestedFiles;
};

#endif
