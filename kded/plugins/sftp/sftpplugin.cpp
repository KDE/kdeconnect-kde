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
#include <QDir>
#include <QTimerEvent>

#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KProcess>
#include <KRun>
#include <KStandardDirs>
#include <kde_file.h>

#include "sftp_config.h"
#include "../../kdebugnamespace.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

static const char* passwd_c = "sftppassword";
static const char* mountpoint_c = "sftpmountpoint";
static const char* timestamp_c = "timestamp";

static const QSet<QString> fields_c = QSet<QString>() <<
  "ip" << "port" << "user" << "port" << "password" << "path";
  

inline bool isTimeout(QObject* o, const KConfigGroup& cfg)
{
    int duration = o->property(timestamp_c).toDateTime().secsTo(QDateTime::currentDateTime());  
    return cfg.readEntry("idle", true) && duration > (cfg.readEntry("idletimeout", 60) * 60);
}

inline QString mountpoint(QObject* o)
{
    return o->property(mountpoint_c).toString();
}

struct SftpPlugin::Pimpl
{
    Pimpl() : waitForMount(false)
    {
        mountTimer.setSingleShot(true);
    };
    
    QPointer<KProcess> mountProc;  
    QTimer mountTimer;
    int idleTimer;
    bool waitForMount;
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_d(new Pimpl)
{ 
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents);
    
    m_d->idleTimer = startTimer(20 * 1000);
    
    connect(&m_d->mountTimer, SIGNAL(timeout()), this, SLOT(mountTimeout()));
}

void SftpPlugin::connected()
{
}

SftpPlugin::~SftpPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
    umount();
}

void SftpPlugin::mount()
{
    if (m_d->mountTimer.isActive() || m_d->mountProc)
    {
        return;
    }
    else
    {
        m_d->mountTimer.start(10000);    
    }

    NetworkPackage np(PACKAGE_TYPE_SFTP);
    np.set("startBrowsing", true);
    device()->sendPackage(np);
}

void SftpPlugin::umount()
{
    if (m_d->mountProc)
    {
        cleanMountPoint(m_d->mountProc);
        if (m_d->mountProc)
        {
            m_d->mountProc->terminate();
            QTimer::singleShot(5000, m_d->mountProc, SLOT(kill()));
            m_d->mountProc->waitForFinished();
        }
    }
}

void SftpPlugin::startBrowsing()
{
    if (m_d->mountProc)
    {
        new KRun(KUrl::fromLocalFile(mountpoint(m_d->mountProc)), 0);
    }
    else
    {
        m_d->waitForMount = true;
        mount();
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
        return true;
    }
    
    m_d->mountTimer.stop();
    
    m_d->mountProc = new KProcess(this);
    m_d->mountProc->setOutputChannelMode(KProcess::SeparateChannels);

    connect(m_d->mountProc, SIGNAL(started()), SLOT(onStarted()));    
    connect(m_d->mountProc, SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), m_d->mountProc, SLOT(deleteLater()));
    
    const QString mpoint = KConfig("kdeconnect/plugins/sftp").group("main").readEntry("mountpoint"
      , KStandardDirs::locateLocal("appdata", "", true, componentData())) + "/" + device()->name() + "/";
    QDir().mkpath(mpoint);
    
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
        << mpoint;
    
    m_d->mountProc->setProgram(program, arguments);
    m_d->mountProc->setProperty(passwd_c, np.get<QString>("password"));
    m_d->mountProc->setProperty(mountpoint_c, mpoint);

    cleanMountPoint(m_d->mountProc);
    m_d->mountProc->start();
    
    return true;
}

void SftpPlugin::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_d->idleTimer) 
    {
        if (isTimeout(m_d->mountProc, SftpConfig::config()->group("main")))
        {
            umount();
        }
    }
    
    QObject::timerEvent(event);
}

void SftpPlugin::onStarted()
{
    m_d->mountProc->setProperty(timestamp_c, QDateTime::currentDateTime());
    
    m_d->mountProc->write(m_d->mountProc->property(passwd_c).toString().toLocal8Bit() + "\n");
    m_d->mountProc->closeWriteChannel();
    
    knotify(KNotification::Notification
        , i18n("Filesystem mounted at %1").arg(mountpoint(sender()))
        , KIconLoader::global()->loadIcon("drive-removable-media", KIconLoader::Desktop)
    );
    
    if (m_d->waitForMount)
    {
        m_d->waitForMount = false;
        new KRun(KUrl::fromLocalFile(mountpoint(sender())), 0);
    }

}

void SftpPlugin::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        knotify(KNotification::Error
            , i18n("Failed to start sshfs")
            , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
        );
        cleanMountPoint(sender());
    }
}

void SftpPlugin::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);

    if (exitStatus == QProcess::NormalExit)
    {
        if (isTimeout(sender(), SftpConfig::config()->group("main")))      
        {
            knotify(KNotification::Notification
                , i18n("Filesystem unmounted by idle timeout")
                , KIconLoader::global()->loadIcon("clock", KIconLoader::Desktop)
            );
        }
        else
        {
            knotify(KNotification::Notification
                , i18n("Filesystem unmounted")
                , KIconLoader::global()->loadIcon("dialog-ok", KIconLoader::Desktop)
            );
        }
    }
    else
    {
        knotify(KNotification::Error
            , i18n("Error when accessing to filesystem")
            , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
        );
    }

    cleanMountPoint(sender());   
    m_d->mountProc = 0;
}

void SftpPlugin::knotify(int type, const QString& text, const QPixmap& icon) const
{
    KNotification::event(KNotification::StandardEvent(type)
      , i18n("Device %1").arg(device()->name()), text, icon, 0
      , KNotification::CloseOnTimeout);
}

void SftpPlugin::cleanMountPoint(QObject* mounter)
{
    if (!mounter || mountpoint(mounter).isEmpty()) 
    {
        return;
    }
    
    KProcess::execute(QStringList()
        << "fusermount" << "-u"
        << mountpoint(mounter), 10000);
}

void SftpPlugin::mountTimeout()
{
    knotify(KNotification::Error
        , i18n("Failed to mount filesystem: device not responding")
        , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
    );
}

