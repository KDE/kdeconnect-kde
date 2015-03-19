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
#include <QDebug>

#include <KLocalizedString>

#include "mounter.h"
#include "sftp_debug.h"
#include <kdeconnectconfig.h>

Mounter::Mounter(SftpPlugin* sftp)
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

    m_connectTimer.setInterval(10000);
    m_connectTimer.setSingleShot(true);

    QTimer::singleShot(0, this, SLOT(start()));
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Created";
}

Mounter::~Mounter()
{
    unmount();
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Destroyed";
}

bool Mounter::wait()
{
    if (m_started)
    {
        return true;
    }
    
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Starting loop to wait for mount";
    
    MountLoop loop;
    connect(this, SIGNAL(mounted()), &loop, SLOT(successed()));
    connect(this, SIGNAL(failed(QString)), &loop, SLOT(failed()));
    return loop.exec();
}

void Mounter::onPakcageReceived(const NetworkPackage& np)
{
    if (np.get<bool>("stop", false))
    {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "SFTP server stopped";
        unmount();
        return;
    }
    
    //TODO implement on android side
    //if (np.get<int>("id") != m_id) return;

    //This is the previous code, to access sftp server using KIO. Now we are
    //using the external binary sshfs, and accessing it as a local filesystem.
  /*
   *    QUrl url;
   *    url.setScheme("sftp");
   *    url.setHost(np.get<QString>("ip"));
   *    url.setPort(np.get<QString>("port").toInt());
   *    url.setUserName(np.get<QString>("user"));
   *    url.setPassword(np.get<QString>("password"));
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

    QString path;
    if (np.has("multiPaths")) path = "/";
    else path = np.get<QString>("path");

    const QStringList arguments = QStringList()
        << QString("%1@%2:%3")
            .arg(np.get<QString>("user"))
            .arg(np.get<QString>("ip"))
            .arg(path)
        << mpoint            
        << "-p" << np.get<QString>("port")
        << "-d"
        << "-f"
        << "-o" << "IdentityFile=" + KdeConnectConfig::instance()->privateKeyPath()
        << "-o" << "StrictHostKeyChecking=no" //Do not ask for confirmation because it is not a known host
        << "-o" << "UserKnownHostsFile=/dev/null"; //Prevent storing as a known host
    
    m_proc->setProgram(program, arguments);

    //To debug
    //m_proc->setStandardOutputFile("/tmp/kdeconnect-sftp.out");
    //m_proc->setStandardErrorFile("/tmp/kdeconnect-sftp.err");

    cleanMountPoint();
    
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Staring process: " << m_proc->program().join(" ");
    m_proc->start();
}

void Mounter::onStarted()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process started";
    m_started = true;
    Q_EMIT mounted();
    
    connect(m_proc.data(), SIGNAL(readyReadStandardError()), this, SLOT(readProcessOut()));
    connect(m_proc.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessOut()));
    
    m_lastActivity = QDateTime::currentDateTime();
}

void Mounter::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process failed to start";
        m_started = false;
        Q_EMIT failed(i18n("Failed to start sshfs"));
    }
}

void Mounter::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process finished (exit code: " << exitCode << ")";
        Q_EMIT unmounted();
    }
    else
    {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process failed (exit code: " << exitCode << ")";
        Q_EMIT failed(i18n("Error when accessing to filesystem"));
    }
    
    cleanMountPoint();
    m_proc.reset();
    m_started = false;  
}

void Mounter::onMountTimeout()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Timeout: device not responding";
    Q_EMIT failed(i18n("Failed to mount filesystem: device not responding"));
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

