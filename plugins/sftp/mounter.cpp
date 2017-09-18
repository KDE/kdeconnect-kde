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

#include "mounter.h"

#include <unistd.h>
#include <QDir>
#include <QDebug>

#include <KLocalizedString>

#include "mountloop.h"
#include "config-sftp.h"
#include "sftp_debug.h"
#include "kdeconnectconfig.h"

Mounter::Mounter(SftpPlugin* sftp)
    : QObject(sftp)
    , m_sftp(sftp)
    , m_proc(nullptr)
    , m_mountPoint(sftp->mountPoint())
    , m_started(false)
{

    connect(m_sftp, &SftpPlugin::packageReceived, this, &Mounter::onPakcageReceived);

    connect(&m_connectTimer, &QTimer::timeout, this, &Mounter::onMountTimeout);

    connect(this, &Mounter::mounted, &m_connectTimer, &QTimer::stop);
    connect(this, &Mounter::failed, &m_connectTimer, &QTimer::stop);

    m_connectTimer.setInterval(10000);
    m_connectTimer.setSingleShot(true);

    QTimer::singleShot(0, this, &Mounter::start);
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Created mounter";
}

Mounter::~Mounter()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Destroy mounter";
    unmount(false);
}

bool Mounter::wait()
{
    if (m_started)
    {
        return true;
    }

    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Starting loop to wait for mount";

    MountLoop loop;
    connect(this, &Mounter::mounted, &loop, &MountLoop::successed);
    connect(this, &Mounter::failed, &loop, &MountLoop::failed);
    return loop.exec();
}

void Mounter::onPakcageReceived(const NetworkPackage& np)
{
    if (np.get<bool>(QStringLiteral("stop"), false))
    {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "SFTP server stopped";
        unmount(false);
        return;
    }

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

    unmount(false);

    m_proc = new KProcess();
    m_proc->setOutputChannelMode(KProcess::MergedChannels);

    connect(m_proc, &QProcess::started, this, &Mounter::onStarted);
    connect(m_proc, SIGNAL(error(QProcess::ProcessError)), SLOT(onError(QProcess::ProcessError)));
    connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onFinished(int,QProcess::ExitStatus)));

    QDir().mkpath(m_mountPoint);

    const QString program = QStringLiteral("sshfs");

    QString path;
    if (np.has(QStringLiteral("multiPaths"))) path = '/';
    else path = np.get<QString>(QStringLiteral("path"));

    QHostAddress addr = m_sftp->device()->getLocalIpAddress();
    if (addr == QHostAddress::Null) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Device doesn't have a LanDeviceLink, unable to get IP address";
        return;
    }
    QString ip = addr.toString();

    const QStringList arguments = QStringList()
        << QStringLiteral("%1@%2:%3").arg(
            np.get<QString>(QStringLiteral("user")),
            ip,
            path)
        << m_mountPoint
        << QStringLiteral("-p") << np.get<QString>(QStringLiteral("port"))
        << QStringLiteral("-s") // This fixes a bug where file chunks are sent out of order and get corrupted on reception
        << QStringLiteral("-f")
        << QStringLiteral("-F") << QStringLiteral("/dev/null") //Do not use ~/.ssh/config
        << QStringLiteral("-o") << "IdentityFile=" + KdeConnectConfig::instance()->privateKeyPath()
        << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no") //Do not ask for confirmation because it is not a known host
        << QStringLiteral("-o") << QStringLiteral("UserKnownHostsFile=/dev/null") //Prevent storing as a known host
        << QStringLiteral("-o") << QStringLiteral("HostKeyAlgorithms=ssh-dss") //https://bugs.kde.org/show_bug.cgi?id=351725
        << QStringLiteral("-o") << QStringLiteral("uid=") + QString::number(getuid())
        << QStringLiteral("-o") << QStringLiteral("gid=") + QString::number(getgid())
        << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=30")
        << QStringLiteral("-o") << QStringLiteral("password_stdin")
        ;

    m_proc->setProgram(program, arguments);

    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Starting process: " << m_proc->program().join(QStringLiteral(" "));
    m_proc->start();

    //qCDebug(KDECONNECT_PLUGIN_SFTP) << "Passing password: " << np.get<QString>("password").toLatin1();
    m_proc->write(np.get<QString>(QStringLiteral("password")).toLatin1());
    m_proc->write("\n");

}

void Mounter::onStarted()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process started";
    m_started = true;
    Q_EMIT mounted();

    //m_proc->setStandardOutputFile("/tmp/kdeconnect-sftp.out");
    //m_proc->setStandardErrorFile("/tmp/kdeconnect-sftp.err");

    auto proc = m_proc;
    connect(m_proc, &KProcess::readyReadStandardError, [proc]() {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "stderr: " << proc->readAll();
    });
    connect(m_proc, &KProcess::readyReadStandardOutput, [proc]() {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "stdout:" << proc->readAll();
    });
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
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process failed (exit code:" << exitCode << ")";
        Q_EMIT failed(i18n("Error when accessing to filesystem"));
    }

    unmount(true);
}

void Mounter::onMountTimeout()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Timeout: device not responding";
    Q_EMIT failed(i18n("Failed to mount filesystem: device not responding"));
}

void Mounter::start()
{
    NetworkPackage np(PACKAGE_TYPE_SFTP_REQUEST, {{"startBrowsing", true}});
    m_sftp->sendPackage(np);

    m_connectTimer.start();
}

void Mounter::unmount(bool finished)
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Unmount" << m_proc;
    if (m_proc)
    {
        if (!finished)
        {
            //Process is still running, we want to stop it
            //But when the finished signal come, we might have already gone.
            //Disconnect everything.
            m_proc->disconnect();
            m_proc->kill();

            auto proc = m_proc;
            m_proc = nullptr;
            connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [proc]() {
                    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Free" << proc;
                    proc->deleteLater();
            });
            Q_EMIT unmounted();
        }
        else
            m_proc->deleteLater();

        //Free mount point (won't always succeed if the path is in use)
#if defined(HAVE_FUSERMOUNT)
        KProcess::execute(QStringList() << QStringLiteral("fusermount") << QStringLiteral("-u") << m_mountPoint, 10000);
#else
        KProcess::execute(QStringList() << QStringLiteral("umount") << m_mountPoint, 10000);
#endif
        m_proc = nullptr;
    }
    m_started = false;
}
