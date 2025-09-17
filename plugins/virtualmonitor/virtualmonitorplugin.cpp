/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2024 Fabian Arndt <fabian.arndt@root-core.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "virtualmonitorplugin.h"

#include <KLocalizedString>
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

class EmitActive
{
public:
    EmitActive(VirtualMonitorPlugin *p)
        : p(p)
        , wasActive(p->active())
    {
        if (!s_outmost) {
            s_outmost = this;
            isOutmost = true;
        } else {
            isOutmost = false;
        }
    }
    ~EmitActive()
    {
        if (isOutmost) {
            s_outmost = nullptr;
            if (p->active() != wasActive) {
                Q_EMIT p->activeChanged();
            }
        }
    }

private:
    static EmitActive *s_outmost;
    bool isOutmost;
    VirtualMonitorPlugin *const p;
    const bool wasActive;
};

EmitActive *EmitActive::s_outmost = nullptr;

VirtualMonitorPlugin::~VirtualMonitorPlugin()
{
    stop();
}

bool VirtualMonitorPlugin::active() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void VirtualMonitorPlugin::stop()
{
    EmitActive emitter(this);
    if (!m_process)
        return;

    // Disconnect the slot to avoid re-entrancy and calling requestVirtualMonitor during destruction.
    QObject::disconnect(m_process, &QProcess::finished, this, nullptr);

    m_process->terminate();
    if (!m_process->waitForFinished()) {
        m_process->kill();
        m_process->waitForFinished();
    }
    m_process->deleteLater();
    m_process = nullptr;
}

void VirtualMonitorPlugin::connected()
{
    // Get local capabilities
    // We test again, since the user may have installed the necessary packages in the meantime
    m_capabilitiesLocal.rdpClient = checkRdpClient();
    m_capabilitiesLocal.virtualMonitor = !QStandardPaths::findExecutable(QS("krdpserver")).isEmpty();
    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Local device supports RDP: " << m_capabilitiesLocal.rdpClient;
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
                         {QS("supports_rdp"), m_capabilitiesLocal.rdpClient},
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
        url.setScheme(received.get<QString>(QS("protocol"), QS("rdp")));
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
            m_lastError = received.get<QString>(QS("failed"));
            stop();
        }

        // Get device's capabilities
        m_capabilitiesRemote.rdpClient = received.get<bool>(QS("supports_rdp"), false);
        m_capabilitiesRemote.virtualMonitor = received.get<bool>(QS("supports_virt_mon"), false);
        qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Remote device supports RDP:" << m_capabilitiesRemote.rdpClient;
        qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Remote device supports Virtual Monitor:" << m_capabilitiesRemote.virtualMonitor << m_remoteResolution;
    }
}

QString VirtualMonitorPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/virtualmonitor").arg(device()->id());
}

bool VirtualMonitorPlugin::requestVirtualMonitor()
{
    EmitActive emitter(this);

    stop();
    m_lastError.clear();
    auto addError = [&](QStringView error) {
        if (!m_lastError.isEmpty()) {
            m_lastError.append(u"\n\n");
        }
        m_lastError.append(error);
    };

    if (!hasRemoteClient()) {
        addError(i18n("Remote device does not have a VNC or RDP client (eg. krdc) installed."));
    }
    if (!isVirtualMonitorAvailable()) {
        addError(i18n("The krdp package is required on the local device."));
    }
    if (m_remoteResolution.isEmpty()) {
        addError(i18n("Cannot start a request without a resolution"));
    }
    if (!m_lastError.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Cannot request virtual display" << device()->name() << m_lastError;
        return false;
    }

    bool ret = false;
    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Requesting display" << m_capabilitiesRemote.rdpClient;
    if (m_capabilitiesRemote.rdpClient && m_capabilitiesLocal.virtualMonitor) {
        ret = requestRdp();
    }

    qCDebug(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Requested virtual display " << ret << m_lastError;
    return ret;
}

bool VirtualMonitorPlugin::requestRdp()
{
    // Cutting by 8 because rdp gets confused when the password is longer
    QString password = QUuid::createUuid().toString(QUuid::Id128).left(20);
    static uint s_port = DEFAULT_PORT;
    const QString port = QString::number(s_port++);

    auto process = new QProcess(this);
    process->setProgram(QS("krdpserver"));
    const double scale = m_remoteResolution.value(QLatin1String("scale")).toDouble();
    QStringList args = {QS("--virtual-monitor"),
                        m_remoteResolution.value(QLatin1String("resolution")).toString() + u'@' + QString::number(scale),
                        QS("--username"),
                        QS("user"),
                        QS("--password"),
                        password,
                        QS("--port"),
                        port};

    static const bool s_plasma = qgetenv("XDG_CURRENT_DESKTOP").compare("KDE", Qt::CaseInsensitive) == 0;
    if (s_plasma) {
        args << QS("--plasma");
    }
    process->setArguments(args);
    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "krdp virtual display finished with" << device()->name() << process->readAllStandardError();
        if (m_retries < 5 && (exitCode == 1 || exitStatus == QProcess::CrashExit)) {
            m_retries++;
            requestRdp();
        } else {
            EmitActive emitter(this);
            process->deleteLater();
            if (m_process == process) {
                m_process = nullptr;
            }
        }
    });

    m_process = process;
    m_process->start();

    if (!m_process->waitForStarted()) {
        qCWarning(KDECONNECT_PLUGIN_VIRTUALMONITOR) << "Failed to start krfb-virtualmonitor" << m_process->error() << m_process->errorString();
        m_lastError = m_process->errorString();
        return false;
    }

    NetworkPacket np(PACKET_TYPE_VIRTUALMONITOR_REQUEST);
    np.set(QS("protocol"), QS("rdp"));
    np.set(QS("username"), QS("user"));
    np.set(QS("password"), password);
    np.set(QS("port"), port);
    sendPacket(np);
    return true;
}

bool VirtualMonitorPlugin::checkRdpClient() const
{
    // krdc is default on KDE
    if (!QStandardPaths::findExecutable(QS("krdc")).isEmpty())
        return true;

    // Check if another client is available
    return checkDefaultSchemeHandler(QS("rdp"));
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
