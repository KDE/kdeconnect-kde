/**
 * Copyright 2016 Holger Kaelberer <holger.k@elberer.de>
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

#ifndef REMOTEKEYBOARDPLUGIN_H
#define REMOTEKEYBOARDPLUGIN_H

#include <core/kdeconnectplugin.h>
#include <QDBusInterface>
#include <QLoggingCategory>
#include <QVariantMap>

struct FakeKey;

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_REMOTEKEYBOARD);

#define PACKAGE_TYPE_REMOTEKEYBOARD_REQUEST QLatin1String("kdeconnect.remotekeyboard.request")
#define PACKAGE_TYPE_REMOTEKEYBOARD QLatin1String("kdeconnect.remotekeyboard")

class RemoteKeyboardPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotekeyboard")

public:
    explicit RemoteKeyboardPlugin(QObject *parent, const QVariantList &args);
    ~RemoteKeyboardPlugin() override;

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override;

    Q_SCRIPTABLE void sendKeyPress(const QString& key, int specialKey = 0,
                                   bool shift = false, bool ctrl = false,
                                   bool alt = false, bool sendAck = true) const;
    Q_SCRIPTABLE void sendQKeyEvent(const QVariantMap& keyEvent,
                                    bool sendAck = true) const;
    Q_SCRIPTABLE int translateQtKey(int qtKey) const;

Q_SIGNALS:
    Q_SCRIPTABLE void keyPressReceived(const QString& key, int specialKey = 0,
                                       bool shift = false, bool ctrl = false,
                                       bool alt = false) const;

private:
    QString dbusPath() const;
};

#endif
