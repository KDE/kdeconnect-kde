/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "virtualmonitorplugin.h"

#include <KPluginFactory>

#include "plugin_virtualmonitor_debug.h"
#include <QDesktopServices>
#include <QGuiApplication>
#include <QJsonArray>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>

K_PLUGIN_CLASS_WITH_JSON(VirtualMonitorPlugin, "kdeconnect_virtualmonitor.json")
#define QS QLatin1String
static const int DEFAULT_PORT = 5901;

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
    NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR,
                     {
                         {QS("resolutions"),
                          QJsonArray{QJsonObject{
                              {QS("resolution"), resolutionString},
                              {QS("scale"), screen->devicePixelRatio()},
                          }}},
                     });
    sendPacket(np);
}

void VirtualMonitorPlugin::receivePacket(const NetworkPacket &received)
{
    if (received.type() == PACKET_TYPE_VIRTUALMONITOR_REQUEST) {
        // At least a password is necessary, we have defaults for all other parameters
        if (!received.has(QS("password"))) {
            qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Request invalid, missing password";
            return;
        }

        // Try to get the IP address of the paired device
        QHostAddress addr = device()->getLocalIpAddress();
        if (addr == QHostAddress::Null) {
            qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Device doesn't have a LanDeviceLink, unable to get IP address";
            return;
        }

        QUrl url;
        url.setScheme(received.get<QString>(QS("protocol"), QS("vnc")));
        url.setUserName(received.get<QString>(QS("username"), QS("user")));
        url.setPassword(received.get<QString>(QS("password")));
        url.setPort(received.get<int>(QS("port"), DEFAULT_PORT));
        url.setHost(addr.toString());

        qCInfo(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Received request, try connecting to" << url.toDisplayString();

        if (!QDesktopServices::openUrl(url)) {
            qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Failed to open" << url.toDisplayString();
            NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR, {{QS("failed"), 0}});
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
}

QString VirtualMonitorPlugin::dbusPath() const
{
    // Don't offer the feature if krfb-virtualmonitor is not around
    if (QStandardPaths::findExecutable(QS("krfb-virtualmonitor")).isEmpty())
        return {};

    return QLatin1String("/modules/kdeconnect/devices/%1/virtualmonitor").arg(device()->id());
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
    static int s_port = DEFAULT_PORT;
    const QString port = QString::number(s_port++);

    m_process = new QProcess(this);
    m_process->setProgram(QS("krfb-virtualmonitor"));
    const double scale = m_remoteResolution.value(QLatin1String("scale")).toDouble();
    m_process->setArguments({QS("--name"),
                             device()->name(),
                             QS("--resolution"),
                             m_remoteResolution.value(QLatin1String("resolution")).toString(),
                             QS("--scale"),
                             QString::number(scale),
                             QS("--password"),
                             uuid.toString(),
                             QS("--port"),
                             port});
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "virtual display finished with" << device()->name() << m_process->readAllStandardError();

        if (m_retries < 5 && (exitCode == 1 || exitStatus == QProcess::CrashExit)) {
            m_retries++;
            requestVirtualMonitor();
        } else {
            m_process->deleteLater();
            m_process = nullptr;
        }
    });

    m_process->start();

    if (!m_process->waitForStarted()) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Failed to start krfb-virtualmonitor" << m_process->error() << m_process->errorString();
        return false;
    }

    NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR_REQUEST);
    np.set(QS("protocol"), QS("vnc"));
    np.set(QS("username"), QS("user"));
    np.set(QS("password"), uuid.toString());
    np.set(QS("port"), port);
    sendPacket(np);
    return true;
}

#include "moc_virtualmonitorplugin.cpp"
#include "virtualmonitorplugin.moc"
