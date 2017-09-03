/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef REMOTECONTROLPLUGIN_H
#define REMOTECONTROLPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKAGE_TYPE_MOUSEPAD_REQUEST QStringLiteral("kdeconnect.mousepad.request")

class Q_DECL_EXPORT RemoteControlPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotecontrol")

public:
    explicit RemoteControlPlugin(QObject* parent, const QVariantList &args);
    ~RemoteControlPlugin() override;

    bool receivePackage(const NetworkPackage& /*np*/) override { return false; }
    void connected() override {}
    QString dbusPath() const override;

    Q_SCRIPTABLE void moveCursor(const QPoint &p);
    Q_SCRIPTABLE void sendCommand(const QString &name, bool val);
};

#endif
