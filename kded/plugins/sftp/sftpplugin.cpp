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

#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KProcess>
#include <KRun>
#include <KStandardDirs>

#include "../../kdebugnamespace.h"
#include "sftpdbusinterface.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

static const char* mount_c = "sftpmount";
static const char* passwd_c = "sftppassword";

static const QSet<QString> fields_c = QSet<QString>() <<
  "ip" << "port" << "user" << "port" << "password" << "path";
  

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , sftpDbusInterface(new SftpDbusInterface(parent))
    , mountProc(0)
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
    stopBrowsing();
}

void SftpPlugin::startBrowsing()
{
    NetworkPackage np(PACKAGE_TYPE_SFTP);
    np.set("startBrowsing", true);
    device()->sendPackage(np);
}

void SftpPlugin::stopBrowsing()
{
    if (mountProc)
    {
        mountProc->terminate();
        QTimer::singleShot(15000, mountProc, SLOT(kill()));
        mountProc->waitForFinished();
    }
}

bool SftpPlugin::receivePackage(const NetworkPackage& np)
{
    if (!mountProc.isNull()) {
        return false;
    }
    
    if (!(fields_c - np.body().keys().toSet()).isEmpty()) {
        // package is invalid
        return false;
    }
    
    mountProc = new KProcess(this);
    mountProc->setOutputChannelMode(KProcess::SeparateChannels);

    connect(mountProc, SIGNAL(started()), SLOT(onStarted()));    
    connect(mountProc, SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), mountProc, SLOT(deleteLater()));
    
    
    const QString mount = KStandardDirs::locateLocal("appdata", device()->name() + "/", true,
      KComponentData("kdeconnect", "kdeconnect"));
    const QString program = "sshfs";
    const QStringList arguments = QStringList() 
      << QString("%1@%2:%3")
        .arg(np.get<QString>("user"))
        .arg(np.get<QString>("ip"))
        .arg(np.get<QString>("path"))
      << "-p" << np.get<QString>("port")
      << "-d"
      << "-f"
      << "-o" << "password_stdin"
      << mount;
    
    mountProc->setProgram(program, arguments);
    mountProc->setProperty(passwd_c, np.get<QString>("password"));
    mountProc->setProperty(mount_c, mount);

    mountProc->start();
    
    return true;
}

void SftpPlugin::onStarted()
{
    mountProc->write(mountProc->property(passwd_c).toString().toLocal8Bit() + "\n");
    mountProc->closeWriteChannel();
    
    KNotification::event(KNotification::Notification
      , i18n("Device %1").arg(device()->name())
      , i18n("Filesystem mounted at %1").arg(mountProc->property(mount_c).toString())
      , KIconLoader::global()->loadIcon("drive-removable-media", KIconLoader::Desktop)
      , 0, KNotification::CloseOnTimeout
    )->sendEvent();
    
    new KRun(KUrl::fromLocalFile(mountProc->property(mount_c).toString()), 0);
}

void SftpPlugin::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        KNotification::event(KNotification::Error
          , i18n("Device %1").arg(device()->name())
          , i18n("Failed to start sshfs")
          , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
          , 0, KNotification::CloseOnTimeout
        )->sendEvent();
    }
}

void SftpPlugin::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    if (exitStatus == QProcess::NormalExit)
    {
        KNotification::event(KNotification::Notification
          , i18n("Device %1").arg(device()->name())
          , i18n("Filesystem unmounted")
          , KIconLoader::global()->loadIcon("dialog-ok", KIconLoader::Desktop)
          , 0, KNotification::CloseOnTimeout
        )->sendEvent();
    }
    else
    {
        KNotification::event(KNotification::Error
          , i18n("Device %1").arg(device()->name())
          , i18n("Error when accessing to filesystem")
          , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
          , 0, KNotification::CloseOnTimeout
        )->sendEvent();
    }
}
