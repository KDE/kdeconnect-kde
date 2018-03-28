/**
 * Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
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

#ifndef FINDTHISDEVICEPLUGIN_H
#define FINDTHISDEVICEPLUGIN_H

#include <core/kdeconnectplugin.h>
// Qt
#include <QLoggingCategory>

#define PACKET_TYPE_FINDMYPHONE_REQUEST QStringLiteral("kdeconnect.findmyphone.request")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_FINDTHISDEVICE)

class FindThisDevicePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.findthisdevice")

public:
    explicit FindThisDevicePlugin(QObject* parent, const QVariantList& args);
    ~FindThisDevicePlugin() override;

    QString dbusPath() const override;
    void connected() override;
    bool receivePacket(const NetworkPacket& np) override;
};

#endif
