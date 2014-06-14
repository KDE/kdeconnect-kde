/**
 * Copyright 2014 Samoilenko Yuri<kinnalru@gmail.com>
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

#include <QDir>
#include <QTimerEvent>

#include <KLocalizedString>
#include <KStandardDirs>

#include <core/kdebugnamespace.h>

#include "mounter.h"

static const char* idleTimeout_c = "idleTimeout";

Mounter::Mounter(SftpPlugin* sftp, int idleTimeout)
    : QObject(sftp)
    , m_sftp(sftp)
    , m_proc(0)
    , m_id(generateId())
    , m_mpoint(m_sftp->mountPoint())
    , m_started(false)
{ 
    Q_ASSERT(sftp);
  
    connect(m_sftp, SIGNAL(packageReceived(NetworkPackage)), this, SLOT(onPakcageReceived(NetworkPackage)));

    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(onMountTimeout()));

    connect(this, SIGNAL(mounted()), &m_connectTimer, SLOT(stop()));
    connect(this, SIGNAL(failed(QString)), &m_connectTimer, SLOT(stop()));

    if (idleTimeout)
    {
        connect(&m_idleTimer, SIGNAL(timeout()), this, SLOT(onIdleTimeout()));
    }

    m_connectTimer.setInterval(10000);
    m_connectTimer.setSingleShot(true);
    
    m_idleTimer.setInterval(idleTimeout);
    m_idleTimer.setSingleShot(false);
    
    QTimer::singleShot(0, this, SLOT(start()));
    kDebug(kdeconnect_kded()) << "Created";
}

Mounter::~Mounter()
{
    unmount();
    kDebug(kdeconnect_kded()) << "Destroyed";
}

bool Mounter::wait()
{
    if (m_started)
    {
        return true;
    }
    
    kDebug(kdeconnect_kded()) << "Starting loop to wait for mount";
    
    MountLoop loop;
    connect(this, SIGNAL(mounted()), &loop, SLOT(successed()));
    connect(this, SIGNAL(failed(QString)), &loop, SLOT(failed()));
    return loop.exec();
}

void Mounter::onPakcageReceived(const NetworkPackage& np)
{
    if (np.get<bool>("stop", false))
    {
        kDebug(kdeconnect_kded()) << "SFTP server stopped";
        unmount();
        return;
    }
    
    //TODO implement on android side
    //if (np.get<int>("id") != m_id) return;
  

    //This is the previous code, to access sftp server using KIO. Now we are
    //using the external binary sshfs, and accessing it as a local filesystem.
  /*
   *    KUrl url;
   *    url.setProtocol("sftp");
   *    url.setHost(np.get<QString>("ip"));
   *    url.setPort(np.get<QString>("port").toInt());
   *    url.setUser(np.get<QString>("user"));
   *    url.setPass(np.get<QString>("password"));
   *    url.setPath(np.get<QString>("path"));
   *    new KRun(url, 0);
   *    Q_EMIT mounted();
   */

    m_proc.reset(new KProcess(this));
    m_proc->setOutputChannelMode(KProcess::MergedChannels);

    connect(m_proc.data(), SIGNAL(started()), SLOT(onStarted()));    
    connect(m_proc.data(), SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(m_proc.data(), SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));

    const QString mpoint = m_sftp->mountPoint();
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
        << "-o" << "IdentityFile=" + m_sftp->device()->privateKeyPath()
        << "-o" << "StrictHostKeyChecking=no" //Do not ask for confirmation because it is not a known host
        << "-o" << "UserKnownHostsFile=/dev/null"; //Prevent storing as a known host
    
    m_proc->setProgram(program, arguments);

    //To debug
    //m_proc->setStandardOutputFile("/tmp/kdeconnect-sftp.out");
    //m_proc->setStandardErrorFile("/tmp/kdeconnect-sftp.err");

    cleanMountPoint();
    
    kDebug(kdeconnect_kded()) << "Staring process: " << m_proc->program().join(" ");
    m_proc->start();
}

void Mounter::onStarted()
{
    kDebug(kdeconnect_kded()) << "Porcess started";
    m_started = true;
    Q_EMIT mounted();
    
    connect(m_proc.data(), SIGNAL(readyReadStandardError()), this, SLOT(readProcessOut()));
    connect(m_proc.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessOut()));
    
    m_lastActivity = QDateTime::currentDateTime();
    
    if (m_idleTimer.interval())
    {
        m_idleTimer.start();
    }
}

void Mounter::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        kDebug(kdeconnect_kded()) << "Porcess failed to start";
        m_started = false;
        Q_EMIT failed(i18n("Failed to start sshfs"));
    }
}

void Mounter::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
        kDebug(kdeconnect_kded()) << "Process finished (exit code: " << exitCode << ")";
        
        if (m_proc->property(idleTimeout_c).toBool())
        {
            Q_EMIT unmounted(true);
        }
        else
        {
            Q_EMIT unmounted(false);
        }
    }
    else
    {
        kDebug(kdeconnect_kded()) << "Porcess failed (exit code: " << exitCode << ")";
        Q_EMIT failed(i18n("Error when accessing to filesystem"));
    }
    
    cleanMountPoint();
    m_proc.reset();
    m_started = false;  
}

void Mounter::onMountTimeout()
{
    kDebug(kdeconnect_kded()) << "Timeout: device not responding";
    Q_EMIT failed(i18n("Failed to mount filesystem: device not responding"));
}

void Mounter::onIdleTimeout()
{
    Q_ASSERT(m_proc.data());
    
    if (m_lastActivity.secsTo(QDateTime::currentDateTime()) >= m_idleTimer.interval() / 1000)
    {
        kDebug(kdeconnect_kded()) << "Timeout: there is no activity on moutned filesystem";      
        m_proc->setProperty(idleTimeout_c, true);
        unmount();
    }
}

void Mounter::readProcessOut()
{
    Q_ASSERT(m_proc.data());
    m_lastActivity = QDateTime::currentDateTime();
    m_proc->readAll();
}

void Mounter::start()
{
    NetworkPackage np(PACKAGE_TYPE_SFTP);
    np.set("startBrowsing", true);
    np.set("start", true);
    np.set("id", m_id);
    np.set("idleTimeout", m_idleTimer.interval());
    m_sftp->sendPackage(np);
    
    m_connectTimer.start();
}

int Mounter::generateId()
{
    static int id = 0;
    return id++;
}

void Mounter::cleanMountPoint()
{
    KProcess::execute(QStringList() << "fusermount" << "-u" << m_mpoint, 10000);
}

void Mounter::unmount()
{
    m_idleTimer.stop();
    
    if (m_proc)
    {
        cleanMountPoint();
        if (m_proc)
        {
            m_proc->terminate();
            QTimer::singleShot(5000, m_proc.data(), SLOT(kill()));
            m_proc->waitForFinished();
        }
    }
    
    m_started = false;    
}

