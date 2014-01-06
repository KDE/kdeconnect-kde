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

#include "sftpplugin.h"

#include <KNotification>
#include <KIcon>
#include <KLocalizedString>
#include <KRun>
#include <QMessageBox>

#include "../../kdebugnamespace.h"
#include "sftpdbusinterface.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , sftpDbusInterface(new SftpDbusInterface(parent))
{

  connect(sftpDbusInterface, SIGNAL(startBrowsing()), this, SLOT(startBrowsing()));
}

void SftpPlugin::connected()
{
}

SftpPlugin::~SftpPlugin()
{
    //FIXME: Qt dbus does not allow to remove an adaptor! (it causes a crash in
    // the next dbus access to its parent). The implication of not deleting this
    // is that disabling the plugin does not remove the interface (that will
    // return outdated values) and that enabling it again instantiates a second
    // adaptor. This is also a memory leak until the entire device is destroyed.

    //sftpDbusInterface->deleteLater();
}

void SftpPlugin::startBrowsing()
{
    NetworkPackage np(PACKAGE_TYPE_SFTP);
    np.set("startBrowsing", true);
    device()->sendPackage(np);
}


bool SftpPlugin::receivePackage(const NetworkPackage& np)
{
    KUrl url;
    url.setProtocol("sftp");
    url.setHost(np.get<QString>("ip"));
    url.setPort(np.get<QString>("port").toInt());
    url.setUser(np.get<QString>("user"));
    url.setPass(np.get<QString>("password"));
    url.setPath(np.get<QString>("home"));
    
    if (url.isValid()) {
        return KRun::runUrl(url, "inode/vnd.kde.service.sftp-ssh", 0);
    }

    return false;
}

