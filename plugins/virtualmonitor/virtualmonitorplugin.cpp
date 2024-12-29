/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2024 Fabian Arndt <fabian.arndt@root-core.net>
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

#if defined(Q_OS_WIN)
#include <QSettings>
#else
#include <KApplicationTrader>
#endif

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
    // Get local capabilities
    // We test again, since the user may have installed the necessary packages in the meantime
    m_capabilitiesLocal.vncClient = checkVncClient();
    m_capabilitiesLocal.virtualMonitor = !QStandardPaths::findExecutable(QS("krfb-virtualmonitor")).isEmpty();
    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Local device supports VNC:" << m_capabilitiesLocal.vncClient;
    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Local device supports Virtual Monitor:" << m_capabilitiesLocal.virtualMonitor;

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
                         {QS("supports_vnc"), m_capabilitiesLocal.vncClient},
                         {QS("supports_virt_mon"), m_capabilitiesLocal.virtualMonitor},
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

        // Get device's capabilities
        m_capabilitiesRemote.vncClient = received.get<bool>(QS("supports_vnc"), false);
        m_capabilitiesRemote.virtualMonitor = received.get<bool>(QS("supports_virt_mon"), false);
        qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Remote device supports VNC:" << m_capabilitiesRemote.vncClient;
        qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Remote device supports Virtual Monitor:" << m_capabilitiesRemote.virtualMonitor;
    }
}

QString VirtualMonitorPlugin::dbusPath() const
{
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

    QString password = QUuid::createUuid().toString(QUuid::WithoutBraces);
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
                             password,
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
    np.set(QS("password"), password);
    np.set(QS("port"), port);
    sendPacket(np);
    return true;
}

bool VirtualMonitorPlugin::checkVncClient() const
{
#if defined(Q_OS_MAC)
    // TODO: macOS has some VNC client preinstalled.. does it work for us?
    return true;
#else
    // krdc is default on KDE
    if (!QStandardPaths::findExecutable(QS("krdc")).isEmpty())
        return true;

    // Check if another client is available
    return checkDefaultSchemeHandler(QS("vnc"));
#endif
}

bool VirtualMonitorPlugin::checkDefaultSchemeHandler(const QString &scheme) const
{
    // TODO: macOS built-in tool: defaults read com.apple.LaunchServices/com.apple.launchservices.secure
    // A function to use such a program was implemented and later removed in MR !670 (Commit e72f688ae1fad8846c006a3f87d57e507f6dbcb6)
#if defined(Q_OS_WIN)
    // Scheme handlers are stored in the registry on Windows
    // https://learn.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/aa767914(v=vs.85)

    // Check if url handler is present
    QSettings handler(QS("HKEY_CLASSES_ROOT\\") + scheme, QSettings::Registry64Format);
    if (handler.value("URL Protocol").isNull())
        return false;

    // Check if a command is actually present
    QSettings command(QS("HKEY_CLASSES_ROOT\\") + scheme + QS("\\shell\\open\\command"), QSettings::Registry64Format);
    return !command.value("Default").isNull();
#else
    KService::Ptr service = KApplicationTrader::preferredService(QS("x-scheme-handler/") + scheme);
    const QString defaultApp = service ? service->desktopEntryName() : QString();
    return !defaultApp.isEmpty();
#endif
}

#include "moc_virtualmonitorplugin.cpp"
#include "virtualmonitorplugin.moc"
