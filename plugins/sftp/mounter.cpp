/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mounter.h"

#include <QDebug>
#include <QDir>
#include <unistd.h>

#include <KLocalizedString>

#include "config-sftp.h"
#include "kdeconnectconfig.h"
#include "mountloop.h"
#include "plugin_sftp_debug.h"

Mounter::Mounter(SftpPlugin *sftp)
    : QObject(sftp)
    , m_sftp(sftp)
    , m_proc(nullptr)
    , m_mountPoint(sftp->mountPoint())
    , m_started(false)
{
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
    if (m_started) {
        return true;
    }

    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Starting loop to wait for mount";

    MountLoop loop;
    connect(this, &Mounter::mounted, &loop, &MountLoop::succeeded);
    connect(this, &Mounter::failed, &loop, &MountLoop::failed);
    return loop.exec();
}

void Mounter::onPacketReceived(const NetworkPacket &np)
{
    unmount(false);

    m_proc = new KProcess();
    m_proc->setOutputChannelMode(KProcess::MergedChannels);

    connect(m_proc, &QProcess::started, this, &Mounter::onStarted);
    connect(m_proc, &QProcess::errorOccurred, this, &Mounter::onError);
    connect(m_proc, &QProcess::finished, this, &Mounter::onFinished);

    QDir().mkpath(m_mountPoint);

    const QString program = QStringLiteral("sshfs");

    QString path;
    if (np.has(QStringLiteral("multiPaths"))) {
        path = QStringLiteral("/");
    } else {
        path = np.get<QString>(QStringLiteral("path"));
    }

    QHostAddress addr = m_sftp->device()->getLocalIpAddress();
    if (addr == QHostAddress::Null) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Device doesn't have a LanDeviceLink, unable to get IP address";
        return;
    }
    QString ip = addr.toString();
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        ip.prepend(QLatin1Char('['));
        ip.append(QLatin1Char(']'));
    }

    // clang-format off
    const QStringList arguments =
        QStringList() << QStringLiteral("%1@%2:%3").arg(np.get<QString>(QStringLiteral("user")), ip, path)
                      << m_mountPoint << QStringLiteral("-p") << np.get<QString>(QStringLiteral("port"))
                      << QStringLiteral("-s") // This fixes a bug where file chunks are sent out of order and get corrupted on reception
                      << QStringLiteral("-f") << QStringLiteral("-F") << QStringLiteral("/dev/null") // Do not use ~/.ssh/config
                      << QStringLiteral("-o") << QStringLiteral("IdentityFile=") + KdeConnectConfig::instance().privateKeyPath()
                      << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no") // Do not ask for confirmation because it is not a known host
                      << QStringLiteral("-o") << QStringLiteral("UserKnownHostsFile=/dev/null") // Prevent storing as a known host
                      << QStringLiteral("-o") << QStringLiteral("uid=") + QString::number(getuid())
                      << QStringLiteral("-o") << QStringLiteral("gid=") + QString::number(getgid())
                      << QStringLiteral("-o") << QStringLiteral("reconnect")
                      << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=30")
                      << QStringLiteral("-o") << QStringLiteral("password_stdin");
    // clang-format on

    m_proc->setProgram(program, arguments);

    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Starting process: " << m_proc->program().join(QStringLiteral(" "));
    m_proc->start();

    // qCDebug(KDECONNECT_PLUGIN_SFTP) << "Passing password: " << np.get<QString>("password").toLatin1();
    m_proc->write(np.get<QString>(QStringLiteral("password")).toLatin1());
    m_proc->write("\n");
}

void Mounter::onStarted()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process started";
    m_started = true;
    Q_EMIT mounted();

    // m_proc->setStandardOutputFile("/tmp/kdeconnect-sftp.out");
    // m_proc->setStandardErrorFile("/tmp/kdeconnect-sftp.err");

    auto proc = m_proc;
    connect(m_proc, &KProcess::readyReadStandardError, this, [proc]() {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "stderr: " << proc->readAll();
    });
    connect(m_proc, &KProcess::readyReadStandardOutput, this, [proc]() {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "stdout:" << proc->readAll();
    });
}

void Mounter::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "sshfs process failed to start";
        m_started = false;
        Q_EMIT failed(i18n("Failed to start sshfs"));
    } else if (error == QProcess::ProcessError::Crashed) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "sshfs process crashed";
        m_started = false;
        Q_EMIT failed(i18n("sshfs process crashed"));
    } else {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "sshfs process error" << error;
        m_started = false;
        Q_EMIT failed(i18n("Unknown error in sshfs"));
    }
}

void Mounter::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process finished (exit code: " << exitCode << ")";
        Q_EMIT unmounted();
    } else {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Process failed (exit code:" << exitCode << ")";
        Q_EMIT failed(i18n("Error when accessing filesystem. sshfs finished with exit code %0").arg(exitCode));
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
    NetworkPacket np(PACKET_TYPE_SFTP_REQUEST, {{QStringLiteral("startBrowsing"), true}});
    m_sftp->sendPacket(np);

    m_connectTimer.start();
}

void Mounter::unmount(bool finished)
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Unmount" << m_proc;
    if (m_proc) {
        if (!finished) {
            // Process is still running, we want to stop it
            // But when the finished signal come, we might have already gone.
            // Disconnect everything.
            m_proc->disconnect();
            m_proc->kill();

            auto proc = m_proc;
            m_proc = nullptr;
            connect(proc, &QProcess::finished, [proc]() {
                qCDebug(KDECONNECT_PLUGIN_SFTP) << "Free" << proc;
                proc->deleteLater();
            });
            Q_EMIT unmounted();
        } else
            m_proc->deleteLater();

        // Free mount point (won't always succeed if the path is in use)
#if defined(HAVE_FUSERMOUNT)
        KProcess::execute(QStringList{QStringLiteral("fusermount"), QStringLiteral("-u"), m_mountPoint}, 10000);
#else
        KProcess::execute(QStringList{QStringLiteral("umount"), m_mountPoint}, 10000);
#endif
        m_proc = nullptr;
    }
    m_started = false;
}

#include "moc_mounter.cpp"
