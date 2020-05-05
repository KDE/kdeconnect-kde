/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2020 Aditya Mehra <aix.m@outlook.com>
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

#ifndef BIGSCREENPLUGIN_H
#define BIGSCREENPLUGIN_H

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_BIGSCREEN_STT QStringLiteral("kdeconnect.bigscreen.stt")

class Q_DECL_EXPORT BigscreenPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.bigscreen")

public:
    explicit BigscreenPlugin(QObject* parent, const QVariantList& args);
    ~BigscreenPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {};
    QString dbusPath() const override;

Q_SIGNALS:
    Q_SCRIPTABLE void messageReceived(const QString& message);

};

#endif
