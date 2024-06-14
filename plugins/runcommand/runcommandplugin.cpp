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
#include <kcmutils_version.h>

#include <core/daemon.h>
#include <core/device.h>
#include <core/networkpacket.h>
#include <core/openconfig.h>

#include "plugin_runcommand_debug.h"

#define PACKET_TYPE_RUNCOMMAND QStringLiteral("kdeconnect.runcommand")

#ifdef Q_OS_WIN
#define COMMAND "cmd"
#define ARGS "/C"
#else
#define COMMAND "/bin/sh"
#define ARGS "-c"
#endif

K_PLUGIN_CLASS_WITH_JSON(RunCommandPlugin, "kdeconnect_runcommand.json")

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
        QJsonDocument commandsDocument = QJsonDocument::fromJson(config()->getByteArray(QStringLiteral("commands"), "{}"));
        QJsonObject commands = commandsDocument.object();
        QString key = np.get<QString>(QStringLiteral("key"));
        QJsonValue value = commands[key];
        if (value == QJsonValue::Undefined) {
            qCWarning(KDECONNECT_PLUGIN_RUNCOMMAND) << key << "is not a configured command";
        }
        const QJsonObject commandJson = value.toObject();
        qCInfo(KDECONNECT_PLUGIN_RUNCOMMAND) << "Running:" << COMMAND << ARGS << commandJson[QStringLiteral("command")].toString();
        QProcess::startDetached(QStringLiteral(COMMAND), QStringList{QStringLiteral(ARGS), commandJson[QStringLiteral("command")].toString()});
    } else if (np.has(QStringLiteral("setup"))) {
        OpenConfig oc;
        oc.openConfiguration(device()->id(), QStringLiteral("kdeconnect_runcommand"));
    }
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
