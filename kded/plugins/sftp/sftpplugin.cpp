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

#include <QDBusConnection>

#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KProcess>
#include <KRun>
#include <KStandardDirs>

#include "../../kdebugnamespace.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

static const char* passwd_c = "sftppassword";

static const QSet<QString> fields_c = QSet<QString>() <<
  "ip" << "port" << "user" << "port" << "password" << "path";
  

struct SftpPlugin::Pimpl
{
    Pimpl() {};
    
    QString mountPoint;
    QPointer<KProcess> mountProc;  
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_d(new Pimpl)
{ 
    m_d->mountPoint = KStandardDirs::locateLocal("appdata", device()->name() + "/", true,
        KComponentData("kdeconnect", "kdeconnect"));
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents);
}

void SftpPlugin::connected()
{
}

SftpPlugin::~SftpPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
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
    cleanMountPoint();
    if (m_d->mountProc)
    {
        m_d->mountProc->terminate();
        QTimer::singleShot(5000, m_d->mountProc, SLOT(kill()));
        m_d->mountProc->waitForFinished();
    }
}

bool SftpPlugin::receivePackage(const NetworkPackage& np)
{
    if (!(fields_c - np.body().keys().toSet()).isEmpty())
    {
        // package is invalid
        return false;
    }
    
    if (!m_d->mountProc.isNull())
    {
        return new KRun(KUrl::fromLocalFile(m_d->mountPoint), 0);
    }
    
    m_d->mountProc = new KProcess(this);
    m_d->mountProc->setOutputChannelMode(KProcess::SeparateChannels);

    connect(m_d->mountProc, SIGNAL(started()), SLOT(onStarted()));    
    connect(m_d->mountProc, SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), m_d->mountProc, SLOT(deleteLater()));
    
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
        << m_d->mountPoint;
    
    m_d->mountProc->setProgram(program, arguments);
    m_d->mountProc->setProperty(passwd_c, np.get<QString>("password"));

    cleanMountPoint();
    m_d->mountProc->start();
    
    return true;
}

void SftpPlugin::onStarted()
{
    m_d->mountProc->write(m_d->mountProc->property(passwd_c).toString().toLocal8Bit() + "\n");
    m_d->mountProc->closeWriteChannel();
    
    knotify(KNotification::Notification
        , i18n("Device %1").arg(device()->name())
        , i18n("Filesystem mounted at %1").arg(m_d->mountPoint)
        , KIconLoader::global()->loadIcon("drive-removable-media", KIconLoader::Desktop)
    );
    
    new KRun(KUrl::fromLocalFile(m_d->mountPoint), 0);
}

void SftpPlugin::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        knotify(KNotification::Error
            , i18n("Device %1").arg(device()->name())
            , i18n("Failed to start sshfs")
            , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
        );
        cleanMountPoint();
    }
}

void SftpPlugin::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    if (exitStatus == QProcess::NormalExit)
    {
        knotify(KNotification::Notification
            , i18n("Device %1").arg(device()->name())
            , i18n("Filesystem unmounted")
            , KIconLoader::global()->loadIcon("dialog-ok", KIconLoader::Desktop)
        );
    }
    else
    {
        knotify(KNotification::Error
            , i18n("Device %1").arg(device()->name())
            , i18n("Error when accessing to filesystem")
            , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
        );
    }

    cleanMountPoint();   
    m_d->mountProc = 0;
}

void SftpPlugin::knotify(int type, const QString& title, const QString& text, const QPixmap& icon) const
{
    KNotification::event(KNotification::StandardEvent(type), title, text, icon, 0
      , KNotification::CloseOnTimeout);
}

void SftpPlugin::cleanMountPoint()
{
    if (m_d->mountProc.isNull()) return;
    
    KProcess::execute(QStringList() << "fusermount" << "-u" << m_d->mountPoint, 10000);        
}
