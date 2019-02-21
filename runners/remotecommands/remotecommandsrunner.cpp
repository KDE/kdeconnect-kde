/***************************************************************************
 *   Copyright © 2018 Nicolas Fella <nicolas.fella@gmx.de>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "remotecommandsrunner.h"

#include <QDebug>

#include "interfaces/dbusinterfaces.h"

K_EXPORT_PLASMA_RUNNER(installer, RemoteCommandsRunner)

RemoteCommandsRunner::RemoteCommandsRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
    , m_daemonInterface(new DaemonDbusInterface)
{
    Q_UNUSED(args)

    setObjectName(QStringLiteral("Run Commands"));
    setPriority(AbstractRunner::HighestPriority);
}

RemoteCommandsRunner::~RemoteCommandsRunner()
{
}

void RemoteCommandsRunner::match(Plasma::RunnerContext &context)
{
    QDBusReply<QStringList> devicesReply = m_daemonInterface.devices(true, true);

    if (devicesReply.isValid()) {

        for (const QString& deviceId : devicesReply.value()) {

            DeviceDbusInterface deviceInterface(deviceId, this);

            if(!deviceInterface.hasPlugin("kdeconnect_remotecommands")) {
                continue;
            }

            RemoteCommandsDbusInterface remoteCommandsInterface(deviceId, this);

            const auto cmds = QJsonDocument::fromJson(remoteCommandsInterface.commands()).object();

            for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it!=itEnd; ++it) {
                const QJsonObject cont = it->toObject();

                const QString deviceName = deviceInterface.name();
                const QString commandName = cont.value(QStringLiteral("name")).toString();

                if (deviceName.contains(context.query(), Qt::CaseInsensitive) || commandName.contains(context.query(), Qt::CaseInsensitive)) {

                    Plasma::QueryMatch match(this);
                    match.setType(Plasma::QueryMatch::PossibleMatch);
                    match.setId(it.key());
                    match.setIconName(QStringLiteral("kdeconnect"));
                    match.setText(deviceName + ": " + commandName);
                    match.setSubtext(cont.value(QStringLiteral("command")).toString());
                    match.setData(deviceId + "$" + it.key());
                    context.addMatch(match);
                }
            }
        }
    }
}

void RemoteCommandsRunner::run(const Plasma::RunnerContext &/*context*/, const Plasma::QueryMatch &match)
{
    RemoteCommandsDbusInterface remoteCommandsInterface(match.data().toString().split("$")[0], this);

    remoteCommandsInterface.triggerCommand(match.data().toString().split("$")[1]);
}

#include "remotecommandsrunner.moc"
