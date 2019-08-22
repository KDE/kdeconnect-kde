/**
 * Copyright 2019 Piyush Aggarwal<piyushaggarwal002@gmail.com>
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

#ifndef SFTPPLUGIN_WIN_H
#define SFTPPLUGIN_WIN_H

#include <core/kdeconnectplugin.h>
#include <core/device.h>

#define PACKET_TYPE_SFTP_REQUEST QStringLiteral("kdeconnect.sftp.request")

static const QSet<QString> expectedFields = QSet<QString>() << QStringLiteral("ip")
                                                            << QStringLiteral("port")
                                                            << QStringLiteral("user")
                                                            << QStringLiteral("password")
                                                            << QStringLiteral("path");
;
class SftpPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sftp")

public:
    explicit SftpPlugin(QObject* parent, const QVariantList& args);
    ~SftpPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}
    QString dbusPath() const override { return QStringLiteral("/modules/kdeconnect/devices/") + deviceId + QStringLiteral("/sftp"); }

public Q_SLOTS:
    Q_SCRIPTABLE bool startBrowsing();

private:
   QString deviceId; //Storing it to avoid accessing device() from the destructor which could cause a crash

};


#endif
