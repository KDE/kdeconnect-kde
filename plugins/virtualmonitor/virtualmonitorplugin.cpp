/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "virtualmonitorplugin.h"

#include <KPluginFactory>

#include <QDesktopServices>
#include <QGuiApplication>
#include <QJsonArray>
#include <QProcess>
#include <QScreen>
#include "plugin_virtualmonitor_debug.h"

K_PLUGIN_CLASS_WITH_JSON(VirtualMonitorPlugin, "kdeconnect_virtualmonitor.json")
#define QS QLatin1String

VirtualMonitorPlugin::VirtualMonitorPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

VirtualMonitorPlugin::~VirtualMonitorPlugin()
{
    stop();
}

void VirtualMonitorPlugin::stop()
{
    if (!m_process)
        return;

    m_process->terminate();
    if (!m_process->waitForFinished()) {
        m_process->kill();
        m_process->waitForFinished();
    }
    delete m_process;
    m_process = nullptr;
}

void VirtualMonitorPlugin::connected()
{
    auto screen = QGuiApplication::primaryScreen();
    auto resolution = screen->size();
    QString resolutionString = QString::number(resolution.width()) + QLatin1Char('x') + QString::number(resolution.height());
    NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR, {
        { QS("resolutions"), QJsonArray {
            QJsonObject {
                { QS("resolution"), resolutionString },
                { QS("scale"), screen->devicePixelRatio() },
            }
        } },
    });
    sendPacket(np);
}

bool VirtualMonitorPlugin::receivePacket(const NetworkPacket& received)
{
    if (received.type() == PACKET_TYPE_VIRTUALMONITOR_REQUEST && received.has(QS("url"))) {
        QUrl url(received.get<QString>(QS("url")));
        if (!QDesktopServices::openUrl(url)) {
            qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Failed to open" << url.toDisplayString();
            NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR, { { QS("failed"), 0 } });
            sendPacket(np);
        }
    } else if (received.type() == PACKET_TYPE_VIRTUALMONITOR) {
        if (received.has(QS("resolutions"))) {
            m_remoteResolution = received.get<QJsonArray>(QS("resolutions")).first().toObject();
        }
        if (received.has(QS("failed"))) {
            stop();
        }
    }
    return true;
}

QString VirtualMonitorPlugin::dbusPath() const
{
    // Don't offer the feature if krfb-virtualmonitor is not around
    if (QStandardPaths::findExecutable(QS("krfb-virtualmonitor")).isEmpty())
        return {};

    return QS("/modules/kdeconnect/devices/") + device()->id() + QS("/virtualmonitor");
}

bool VirtualMonitorPlugin::requestVirtualMonitor()
{
    stop();
    if (m_remoteResolution.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Cannot start a request without a resolution";
        return false;
    }

    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Requesting virtual display " << device()->name();

    QUuid uuid = QUuid::createUuid();
    static int s_port = 5901;
    const QString port = QString::number(s_port++);

    m_process = new QProcess(this);
    m_process->setProgram(QS("krfb-virtualmonitor"));
    const double scale = m_remoteResolution.value(QLatin1String("scale")).toDouble();
    m_process->setArguments({QS("--name"), device()->name(), QS("--resolution"), m_remoteResolution.value(QLatin1String("resolution")).toString(), QS("--scale"), QString::number(scale), QS("--password"), uuid.toString(), QS("--port"), port});
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this] (int exitCode, QProcess::ExitStatus exitStatus) {
        if (m_retries < 5 && (exitCode == 1 || exitStatus == QProcess::CrashExit)) {
            m_retries++;
            requestVirtualMonitor();
        } else {
            m_process->deleteLater();
        }
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "virtual display finished with" << device()->name() << m_process->readAllStandardError();
    });

    m_process->start();

    if (!m_process->waitForStarted()) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Failed to start krfb-virtualmonitor" << m_process->error() << m_process->errorString();
        return false;
    }

    QUrl url;
    url.setScheme(QS("vnc"));
    url.setUserName(QS("user"));
    url.setPassword(uuid.toString());
    url.setHost(device()->getLocalIpAddress().toString());

    NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR_REQUEST, {
        { QS("url"), url.toEncoded() }
    });
    sendPacket(np);
    return true;
}

#include "virtualmonitorplugin.moc"
