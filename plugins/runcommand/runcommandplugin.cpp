/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "runcommandplugin.h"

#include <KPluginFactory>

#include <QDBusConnection>
#include <QProcess>
#include <QDir>
#include <QSettings>
#include <QJsonDocument>

#ifdef SAILFISHOS
#define KCMUTILS_VERSION 0
#else
#include <KShell>
#include <kcmutils_version.h>
#endif

#include <core/networkpacket.h>
#include <core/device.h>

#include "plugin_runcommand_debug.h"

#define PACKET_TYPE_RUNCOMMAND QStringLiteral("kdeconnect.runcommand")

#ifdef Q_OS_WIN
#define COMMAND "cmd"
#define ARGS "/c"

#else
#define COMMAND "/bin/sh"
#define ARGS "-c"

#endif

K_PLUGIN_CLASS_WITH_JSON(RunCommandPlugin, "kdeconnect_runcommand.json")

RunCommandPlugin::RunCommandPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    connect(config(), &KdeConnectPluginConfig::configChanged, this, &RunCommandPlugin::configChanged);
}

RunCommandPlugin::~RunCommandPlugin()
{
}

bool RunCommandPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("requestCommandList"), false)) {
        sendConfig();
        return true;
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
        QProcess::startDetached(QStringLiteral(COMMAND), QStringList()<< QStringLiteral(ARGS) << commandJson[QStringLiteral("command")].toString());
        return true;
    } else if (np.has(QStringLiteral("setup"))) {
        QProcess::startDetached(QStringLiteral("kdeconnect-settings"), { QStringLiteral("--args"), QString(device()->id() + QStringLiteral(":kdeconnect_runcommand")) });
    }

    return false;
}

void RunCommandPlugin::connected()
{

    sendConfig();
}

void RunCommandPlugin::sendConfig()
{
    QString commands = config()->getString(QStringLiteral("commands"),QStringLiteral("{}"));
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND, {{QStringLiteral("commandList"), commands}});

    #if KCMUTILS_VERSION >= QT_VERSION_CHECK(5, 45, 0)
        np.set<bool>(QStringLiteral("canAddCommand"), true);
    #endif

    sendPacket(np);
}

void RunCommandPlugin::configChanged() {
    sendConfig();
}

#include "runcommandplugin.moc"
