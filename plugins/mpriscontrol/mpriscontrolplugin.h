/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef MPRISCONTROLPLUGIN_H
#define MPRISCONTROLPLUGIN_H

#include <QString>
#include <QHash>
#include <QLoggingCategory>
#include <QDBusServiceWatcher>

#include <core/kdeconnectplugin.h>

#define PACKAGE_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MPRIS)

class MprisControlPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MprisControlPlugin(QObject* parent, const QVariantList& args);

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override { }

private Q_SLOTS:
    void propertiesChanged(const QString& propertyInterface, const QVariantMap& properties);
    void seeked(qlonglong);

private:
    void serviceOwnerChanged(const QString& serviceName, const QString& oldOwner, const QString& newOwner);
    void addPlayer(const QString& ifaceName);
    void removePlayer(const QString& ifaceName);
    void sendPlayerList();
    void mprisPlayerMetadataToNetworkPackage(NetworkPackage& np, const QVariantMap& nowPlayingMap) const;

    QHash<QString, QString> playerList;
    int prevVolume;
    QDBusServiceWatcher* m_watcher;

};

#endif
