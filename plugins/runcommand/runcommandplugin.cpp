/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "runcommandplugin.h"

#include <KPluginFactory>

#include <QDBusConnection>
#include <QDir>
#include <QJsonDocument>
#include <QProcess>
#include <QSettings>

#include <KShell>
#include <QEventLoop>
#include <QTimer>

#include <core/daemon.h>
#include <core/device.h>
#include <core/networkpacket.h>
#include <core/openconfig.h>

#include "plugin_runcommand_debug.h"

#define PACKET_TYPE_RUNCOMMAND QStringLiteral("kdeconnect.runcommand")
#define PACKET_TYPE_RUNCOMMAND_OUTPUT QStringLiteral("kdeconnect.runcommand.output")

#ifdef Q_OS_WIN
#define COMMAND "cmd"
#define ARGS "/C"
#else
#define COMMAND "/bin/sh"
#define ARGS "-c"
#endif

K_PLUGIN_CLASS_WITH_JSON(RunCommandPlugin, "kdeconnect_runcommand.json")

QMetaObject::Connection stderrConn;
QMetaObject::Connection stdoutConn;

RunCommandPlugin::RunCommandPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    connect(config(), &KdeConnectPluginConfig::configChanged, this, &RunCommandPlugin::sendConfig);
}

RunCommandPlugin::~RunCommandPlugin()
{
    currentProcesses.clear();
}

void RunCommandPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<bool>(QStringLiteral("requestCommandList"), false)) {
        sendConfig();
        return;
    }

    if (np.has(QStringLiteral("key"))) {
        startCommand(np);
    } else if (np.has(QStringLiteral("setup"))) {
        OpenConfig oc;
        oc.openConfiguration(device()->id(), QStringLiteral("kdeconnect_runcommand"));
    } else if (np.has(QStringLiteral("stop"))) {
        for (QProcess *process : std::as_const(currentProcesses)) {
            process->terminate(); // will trigger onProcessFinished
        }
    }
}

void RunCommandPlugin::startCommand(const NetworkPacket &np)
{
    static unsigned id = 0;
    if (id == std::numeric_limits<unsigned>::max()) {
        id = 0;
    }
    unsigned int currentId = id++;

    QJsonDocument commandsDocument = QJsonDocument::fromJson(config()->getByteArray(QStringLiteral("commands"), "{}"));
    QJsonObject commands = commandsDocument.object();
    QString key = np.get<QString>(QStringLiteral("key"));
    QJsonValue value = commands[key];
    if (value == QJsonValue::Undefined) {
        qCWarning(KDECONNECT_PLUGIN_RUNCOMMAND) << key << "is not a configured command";
    }
    const QJsonObject commandJson = value.toObject();

    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "Running:" << COMMAND << ARGS << commandJson[QStringLiteral("command")].toString();
    auto *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);

    stderrConn = connect(process, &QProcess::readyReadStandardError, this, [this, currentId] {
        onProcessReadyReadState(currentId, true);
    });
    stderrConn = connect(process, &QProcess::readyReadStandardOutput, this, [this, currentId] {
        onProcessReadyReadState(currentId, false);
    });
    connect(process, &QProcess::finished, this, [this, currentId](int exitCode, QProcess::ExitStatus exitStatus) {
        onProcessFinished(currentId, exitCode, exitStatus);
    });

    currentProcesses[currentId] = process;

    QString command = commandJson[QStringLiteral("command")].toString();
    process->start(QStringLiteral(COMMAND), QStringList{QStringLiteral(ARGS), command});

    NetworkPacket npOutput(PACKET_TYPE_RUNCOMMAND_OUTPUT,
                           {
                               {QStringLiteral("commandStarted"), true},
                               {QStringLiteral("command"), command},
                               {QStringLiteral("id"), currentId},
                           });
    sendPacket(npOutput);
}

void RunCommandPlugin::onProcessFinished(unsigned int id, int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "Finished with exit code: " << exitCode << " and status " << exitStatus;
    NetworkPacket npOutput(PACKET_TYPE_RUNCOMMAND_OUTPUT,
                           {
                               {QStringLiteral("commandFinished"), true},
                               {QStringLiteral("success"), exitCode != EXIT_FAILURE},
                               {QStringLiteral("exitCode"), exitCode},
                               {QStringLiteral("id"), id},
                           });
    sendPacket(npOutput);
    currentProcesses.remove(id);
    sender()->deleteLater();
}

void RunCommandPlugin::onProcessReadyReadState(unsigned int id, const bool isErrorOutput)
{
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }

    if (isErrorOutput) {
        process->setReadChannel(QProcess::StandardError);
    } else {
        process->setReadChannel(QProcess::StandardOutput);
    }

    QTextStream stream(process);
    QList<QString> output;
    QList<QString> empty;
    while (!stream.atEnd()) {
        output.append(stream.readLine());
        if (output.size() == 5) {
            if (isErrorOutput) {
                sendOutput(id, empty, output);
            } else {
                sendOutput(id, output, empty);
            }
            output.clear();
        }
    }
    if (!output.isEmpty()) {
        if (isErrorOutput) {
            sendOutput(id, empty, output);
        } else {
            sendOutput(id, output, empty);
        }
    }
}

void RunCommandPlugin::sendOutput(unsigned int id, const QStringList &standard, const QStringList &error) const
{
    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "Sending stdout: " << standard << " and stderr: " << error;
    NetworkPacket npOutput(PACKET_TYPE_RUNCOMMAND_OUTPUT,
                           {
                               {QStringLiteral("commandOutput"), true},
                               {QStringLiteral("stdout"), standard},
                               {QStringLiteral("stderr"), error},
                               {QStringLiteral("id"), id},
                           });
    sendPacket(npOutput);
}

void RunCommandPlugin::connected()
{
    sendConfig();
}

void RunCommandPlugin::sendConfig()
{
    QString commands = config()->getString(QStringLiteral("commands"), QStringLiteral("{}"));
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND, {{QStringLiteral("commandList"), commands}});
    np.set<bool>(QStringLiteral("canAddCommand"), true);

    sendPacket(np);
}

#include "moc_runcommandplugin.cpp"
#include "runcommandplugin.moc"
