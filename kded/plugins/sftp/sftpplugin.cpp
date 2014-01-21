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

#include <QDir>
#include <QTimerEvent>

#include <QDBusConnection>

#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KProcess>
#include <KRun>
#include <KStandardDirs>
#include <kde_file.h>
#include <kfileplacesmodel.h>

#include "sftp_config.h"
#include "mountloop.h"
#include "../../kdebugnamespace.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

static const char* lastaccess_c = "lastaccess";
static const QSet<QString> fields_c = QSet<QString>() << "ip" << "port" << "user" << "port" << "path";

inline bool isTimeout(QObject* o, const KConfigGroup& cfg)
{
    if (!o) return false;
    
    int duration = o->property(lastaccess_c).toDateTime().secsTo(QDateTime::currentDateTime());  
    return cfg.readEntry("idle", true) && duration > (cfg.readEntry("idletimeout", 60) * 60);
}


struct SftpPlugin::Pimpl
{
    Pimpl()
    {
        connectTimer.setInterval(10 * 1000);
        connectTimer.setSingleShot(true);
        
        //Add KIO entry to Dolphin's Places
        placesModel = new KFilePlacesModel();
    }
  
    QPointer<KProcess> mountProc;  
    KFilePlacesModel*  placesModel;
    
    QTimer connectTimer;  
    int idleTimerId;
    
    MountLoop loop;
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_d(new Pimpl)
{ 
    kDebug(kdeconnect_kded()) << "creating [" << device()->name() << "]...";
    

    m_d->idleTimerId = startTimer(20 * 1000);

    connect(&m_d->connectTimer, SIGNAL(timeout()), this, SLOT(mountTimeout()));
    connect(&m_d->connectTimer, SIGNAL(timeout()), &m_d->loop, SLOT(failed()));

    connect(this, SIGNAL(mount_succesed()), &m_d->loop, SLOT(successed()));
    connect(this, SIGNAL(mount_failed()), &m_d->loop, SLOT(failed()));

    
    addToDolphin();
    kDebug(kdeconnect_kded()) << "created [" << device()->name() << "]";
}

SftpPlugin::~SftpPlugin()
{
    kDebug(kdeconnect_kded()) << "destroying [" << device()->name() << "]...";

    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
    umount();
    removeFromDolphin();
    kDebug(kdeconnect_kded()) << "destroyed [" << device()->name() << "]";
}

void SftpPlugin::addToDolphin()
{
    removeFromDolphin();
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    m_d->placesModel->addPlace(device()->name(), kioUrl, "smartphone");
    kDebug(kdeconnect_kded()) << "add to dolphin";
}

void SftpPlugin::removeFromDolphin()
{
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    QModelIndex index = m_d->placesModel->closestItem(kioUrl);
    while (index.row() != -1) {
        m_d->placesModel->removePlace(index);
        index = m_d->placesModel->closestItem(kioUrl);
    }
}

void SftpPlugin::connected()
{
    kDebug(kdeconnect_kded()) << "exposing DBUS interface: "
        << QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents);
}

void SftpPlugin::mount()
{
    kDebug(kdeconnect_kded()) << "start mounting device:" << device()->name();
    if (m_d->loop.isRunning() || m_d->mountProc)
    {
        return;
    }

    m_d->connectTimer.start();
    
    NetworkPackage np(PACKAGE_TYPE_SFTP);
    np.set("startBrowsing", true);
    device()->sendPackage(np);
}

bool SftpPlugin::mountAndWait()
{
    kDebug(kdeconnect_kded()) << "start mounting device and block:" << device()->name();
    
    if (m_d->mountProc && !m_d->loop.isRunning())
    {
        return true;
    }
    
    if (m_d->loop.isRunning())
    {
        kDebug(kdeconnect_kded()) << "start secondary loop";
        MountLoop loop;
        connect(&m_d->loop, SIGNAL(result(bool)), &loop, SLOT(exitWith(bool)));
        return loop.exec();
    }

    mount();
    
    QTimer mt;
    connect(&mt, SIGNAL(timeout()), &m_d->loop, SLOT(failed()));
    mt.start(15000);
    kDebug(kdeconnect_kded()) << "start primary loop";
    return m_d->loop.exec();
}


void SftpPlugin::umount()
{
    if (m_d->mountProc)
    {
        cleanMountPoint();
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
    if (mountAndWait())
    {
        new KRun(KUrl::fromLocalFile(mountPoint()), 0);
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
    
    m_d->connectTimer.stop();
    
    m_d->mountProc = new KProcess(this);
    m_d->mountProc->setOutputChannelMode(KProcess::MergedChannels);

    connect(m_d->mountProc, SIGNAL(started()), SLOT(onStarted()));    
    connect(m_d->mountProc, SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_d->mountProc, SIGNAL(finished(int,QProcess::ExitStatus)), m_d->mountProc, SLOT(deleteLater()));
    
    const QString mpoint = mountPoint();
    QDir().mkpath(mpoint);
    
    const QString program = "sshfs";
    const QStringList arguments = QStringList() 
        << QString("%1@%2:%3")
            .arg(np.get<QString>("user"))
            .arg(np.get<QString>("ip"))
            .arg(np.get<QString>("path"))
        << mpoint            
        << "-p" << np.get<QString>("port")
        << "-d"
        << "-f"
        << "-o" << "IdentityFile=" + device()->privateKey();
    
    m_d->mountProc->setProgram(program, arguments);

    cleanMountPoint();
    m_d->mountProc->start();
    
    return true;
}

QString SftpPlugin::mountPoint()
{
    const QString defaultMountDir = KStandardDirs::locateLocal("appdata", "", true, componentData());
    const QString mountDir = KConfig("kdeconnect/plugins/sftp").group("main").readEntry("mountpoint", defaultMountDir);
    return mountDir + "/" + device()->id() + "/";
}

void SftpPlugin::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_d->idleTimerId) 
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
    kDebug(kdeconnect_kded()) << qobject_cast<KProcess*>(sender())->program().join(" ");
    
    m_d->mountProc->setProperty(lastaccess_c, QDateTime::currentDateTime());
    
    knotify(KNotification::Notification
        , i18n("Filesystem mounted at %1").arg(mountPoint())
        , KIconLoader::global()->loadIcon("drive-removable-media", KIconLoader::Desktop)
    );
    
    //Used to notify MountLoop about success. 
    Q_EMIT mount_succesed();
    
    connect(m_d->mountProc, SIGNAL(readyReadStandardError()), this, SLOT(readProcessOut()));
    connect(m_d->mountProc, SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessOut()));
}

void SftpPlugin::onError(QProcess::ProcessError error)
{
    kDebug(kdeconnect_kded()) << qobject_cast<KProcess*>(sender())->program(); 
    kDebug(kdeconnect_kded()) << "ARGS: error=" << error;
    
    if (error == QProcess::FailedToStart)
    {
        knotify(KNotification::Error
            , i18n("Failed to start sshfs")
            , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
        );
        cleanMountPoint();
    }
}

void SftpPlugin::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    kDebug(kdeconnect_kded()) << qobject_cast<KProcess*>(sender())->program();
    kDebug(kdeconnect_kded()) << "ARGS: exitCode=" << exitCode << " exitStatus=" << exitStatus;

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

    cleanMountPoint();
    m_d->mountProc = 0;
    
    //Used to notify MountLoop about error. 
    //There is no loop running if mount was succesful!
    Q_EMIT mount_failed();
}

void SftpPlugin::knotify(int type, const QString& text, const QPixmap& icon) const
{
    KNotification::event(KNotification::StandardEvent(type)
      , i18n("Device %1").arg(device()->name()), text, icon, 0
      , KNotification::CloseOnTimeout);
}

void SftpPlugin::cleanMountPoint()
{
    KProcess::execute(QStringList() << "fusermount" << "-u" << mountPoint(), 10000);
}

void SftpPlugin::mountTimeout()
{
    kDebug(kdeconnect_kded()) << "loop.... TIMEOUT";
    knotify(KNotification::Error
        , i18n("Failed to mount filesystem: device not responding")
        , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
    );
}

void SftpPlugin::readProcessOut()
{
    m_d->mountProc->setProperty(lastaccess_c, QDateTime::currentDateTime());    
    m_d->mountProc->readAll();
}

bool SftpPlugin::waitForMount()
{
    if (m_d->loop.isRunning()) 
    {
        MountLoop loop;
        connect(&m_d->loop, SIGNAL(result(bool)), &loop, SLOT(exitWith(bool)));
        
        kDebug(kdeconnect_kded()) << "start secondary loop...";
        bool ret = loop.exec();
        kDebug(kdeconnect_kded()) << "finish secondary loop...:" << ret;
        return ret;
    }
    
    return m_d->mountProc;
}


// void SftpPlugin::startAgent()
// {
//     m_d->agentProc = new KProcess(this);
//     m_d->agentProc->setOutputChannelMode(KProcess::SeparateChannels);
//     connect(m_d->agentProc, SIGNAL(finished(int,QProcess::ExitStatus)), m_d->agentProc, SLOT(deleteLater()));
//     
//     m_d->agentProc->setProgram("ssh-agent", QStringList("-c"));
//     m_d->agentProc->setReadChannel(QProcess::StandardOutput);
// 
//     kDebug(kdeconnect_kded()) << "1";
//     m_d->agentProc->start();
//     if (!m_d->agentProc->waitForFinished(5000))
//     {
//         kDebug(kdeconnect_kded()) << "2";
//         m_d->agentProc->deleteLater();
//         return;
//     }
//     
//     kDebug(kdeconnect_kded()) << "3";
//     m_d->env = QProcessEnvironment::systemEnvironment();
//     QRegExp envrx("setenv (.*) (.*);");
//     
//     kDebug(kdeconnect_kded()) << "4";
//     QByteArray data = m_d->agentProc->readLine();
//     kDebug(kdeconnect_kded()) << "line readed:" << data;
//     if (envrx.indexIn(data) == -1)
//     {
//         kError(kdeconnect_kded()) << "can't start ssh-agent";
//         return;
//     }
//     m_d->env.insert(envrx.cap(1), envrx.cap(2));
// 
//     KProcess add;
//     add.setProgram("ssh-add", QStringList() << device()->privateKey());
//     add.setProcessEnvironment(m_d->env);
//     add.execute(5000);
// }
