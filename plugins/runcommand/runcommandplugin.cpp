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
#include "runcommandoutput.h"

#define PACKET_TYPE_RUNCOMMAND QStringLiteral("kdeconnect.runcommand")

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
        if (currentProcess) {
            currentProcess->terminate();
            currentProcess = nullptr;
        }
    }
}

void RunCommandPlugin::startCommand(const NetworkPacket &np)
{
    if (currentProcess)
    {
        disconnect(stderrConn);
        disconnect(stdoutConn);
        currentProcess = nullptr;
    }

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

    stderrConn = connect(process, &QProcess::readyReadStandardError, this, [this] {onProcessReadyReadState(true);});
    stderrConn = connect(process, &QProcess::readyReadStandardOutput, this, [this] {onProcessReadyReadState(false);});
    connect(process, &QProcess::finished, this, &RunCommandPlugin::onProcessFinished);

    connect(process, &QProcess::finished, process, &QObject::deleteLater);
    currentProcess = process;

    process->start(QStringLiteral(COMMAND), QStringList{QStringLiteral(ARGS), commandJson[QStringLiteral("command")].toString()});
}

void RunCommandPlugin::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "Finished with exit code: " << exitCode << " and status " << exitStatus;
    NetworkPacket npOutput(PACKET_TYPE_RUNCOMMAND_OUTPUT, {{QStringLiteral("commandFinished"), exitCode != EXIT_FAILURE}});
    sendPacket(npOutput);
    currentProcess = nullptr;
}


void RunCommandPlugin::onProcessReadyReadState(const bool isErrorOutput)
{
    auto *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }

    if (isErrorOutput)
    {
        process->setReadChannel(QProcess::StandardError);
    } else
    {
        process->setReadChannel(QProcess::StandardOutput);
    }

    QTextStream stream(process);
    QList<QString> output;
    QList<QString> empty;
    while (!stream.atEnd()) {
        output.append(stream.readLine());
        if (output.size() == 5) {
            if (isErrorOutput)
            {
                sendOutput(empty, output);
            }
            else
            {
                sendOutput(output, empty);
            }
            output.clear();
        }
    }
    if (!output.isEmpty()) {
        if (isErrorOutput)
        {
            sendOutput(empty, output);
        }
        else
        {
            sendOutput(output, empty);
        }
    }
}

void RunCommandPlugin::sendOutput(const QStringList &standard, const QStringList &error) const
{
    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "Sending stdout: " << standard << " and stderr: " << error;
    NetworkPacket npOutput(PACKET_TYPE_RUNCOMMAND_OUTPUT, {{QStringLiteral("stdout"), standard}, {QStringLiteral("stderr"), error}});
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
