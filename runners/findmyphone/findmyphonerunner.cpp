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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "findmyphonerunner.h"

#include <QDebug>
#include <KLocalizedString>

#include "interfaces/dbusinterfaces.h"

K_EXPORT_PLASMA_RUNNER(installer, FindMyPhoneRunner)

FindMyPhoneRunner::FindMyPhoneRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
    , m_daemonInterface(new DaemonDbusInterface)
{
    Q_UNUSED(args)

    setObjectName(QStringLiteral("Find my device"));
    setPriority(AbstractRunner::NormalPriority);
}

FindMyPhoneRunner::~FindMyPhoneRunner()
{
}

void FindMyPhoneRunner::match(Plasma::RunnerContext &context)
{
    QDBusReply<QStringList> devicesReply = m_daemonInterface.devices(true, true);

    if (devicesReply.isValid()) {

        const auto devices = devicesReply.value();
        for (const QString& deviceId : devices) {

            DeviceDbusInterface deviceInterface(deviceId, this);

            if(!deviceInterface.hasPlugin(QStringLiteral("kdeconnect_findmyphone"))) {
                continue;
            }

            QString deviceName = deviceInterface.name();

            if (deviceName.contains(context.query(), Qt::CaseInsensitive) || i18n("Ring").contains(context.query(), Qt::CaseInsensitive) || i18n("Find").contains(context.query(), Qt::CaseInsensitive)) {

                Plasma::QueryMatch match(this);
                match.setType(Plasma::QueryMatch::PossibleMatch);
                match.setId(deviceId);
                match.setIconName(QStringLiteral("kdeconnect"));
                match.setText(i18n("Find %1", deviceName));
                match.setData(deviceId);
                context.addMatch(match);
            }
        }
    }
}

void FindMyPhoneRunner::run(const Plasma::RunnerContext &/*context*/, const Plasma::QueryMatch &match)
{
    FindMyPhoneDeviceDbusInterface findInterface(match.data().toString(), this);

    findInterface.ring();
}

#include "findmyphonerunner.moc"
