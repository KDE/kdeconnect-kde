/**
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#ifndef REMOTECOMMANDSPLUGIN_H
#define REMOTECOMMANDSPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMap>
#include <QPair>
#include <QString>

class Q_DECL_EXPORT RemoteCommandsPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotecommands")
    Q_PROPERTY(QByteArray commands READ commands NOTIFY commandsChanged)

public:
    explicit RemoteCommandsPlugin(QObject* parent, const QVariantList& args);
    ~RemoteCommandsPlugin() override;

    Q_SCRIPTABLE void triggerCommand(const QString& key);
    QByteArray commands() const { return m_commands; }

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override;
    QString dbusPath() const override;

Q_SIGNALS:
    void commandsChanged(const QByteArray& commands);

private:
    void setCommands(const QByteArray& commands);

    QByteArray m_commands;
};

#endif
