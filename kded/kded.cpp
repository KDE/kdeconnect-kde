/**
 * Copyright 2014 Yuri Samoilenko <kinnalru@gmail.com>
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

#include "kded.h"

#include <QTimer>

#include <KPluginFactory>
#include <KStandardDirs>

#include "kdebugnamespace.h"

K_PLUGIN_FACTORY(KdeConnectFactory, registerPlugin<Kded>();)
K_EXPORT_PLUGIN(KdeConnectFactory("kdeconnect", "kdeconnect"))

Kded::Kded(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
    , m_daemon(0)
{
    start();
    kDebug(kdeconnect_kded()) << "kded_kdeconnect started"; 
}

Kded::~Kded()
{
    stop();
    kDebug(kdeconnect_kded()) << "kded_kdeconnect stopped";
}

bool Kded::start()
{
    if (m_daemon) 
    {
        return true;
    }
    
    const QString daemon = KStandardDirs::locate("exe", "kdeconnectd");
    kDebug(kdeconnect_kded()) << "Starting daemon " << daemon;    
    m_daemon = new KProcess(this);
    connect(m_daemon, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));
    connect(m_daemon, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onFinished(int,QProcess::ExitStatus)));
    connect(m_daemon, SIGNAL(finished(int,QProcess::ExitStatus)), m_daemon, SLOT(deleteLater()));
    
    m_daemon->setProgram(daemon);
    m_daemon->setOutputChannelMode(KProcess::SeparateChannels);
    m_daemon->start();
    if (!m_daemon->waitForStarted(2000)) //FIXME: KDEDs should be non-blocking, do we really need to wait for it to start?
    {
        kError(kdeconnect_kded()) << "Can't start " << daemon;
        return false;
    }

    m_daemon->closeReadChannel(KProcess::StandardOutput);
    
    kDebug(kdeconnect_kded()) << "Daemon successfuly started";
    return true;
}

void Kded::stop()
{
    if (m_daemon)
    {
        m_daemon->terminate();
        if (m_daemon->waitForFinished(10000))
        {
            kDebug(kdeconnect_kded()) << "Daemon successfuly stopped";
        }
        else
        {
            m_daemon->kill();
            kWarning(kdeconnect_kded()) << "Daemon  killed";
        }
        m_daemon = 0;
    }
}

bool Kded::restart()
{
    stop();
    return start();
}

void Kded::onError(QProcess::ProcessError errorCode)
{
    kError(kdeconnect_kded()) << "Process error code=" << errorCode;
}

void Kded::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit)
    {
        kError(kdeconnect_kded()) << "Process crashed with code=" << exitCode;
        kError(kdeconnect_kded()) << m_daemon->readAllStandardError();
        kWarning(kdeconnect_kded()) << "Restarting in 5 sec...";
        QTimer::singleShot(5000, this, SLOT(start()));        
    }
    else
    {
        kWarning(kdeconnect_kded()) << "Process finished with code=" << exitCode;
    }
    m_daemon = 0;
}

