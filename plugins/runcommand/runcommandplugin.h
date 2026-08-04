/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <QFile>
#include <QFileSystemWatcher>
#include <QMap>
#include <QPair>
#include <QProcess>
#include <QString>
#include <core/kdeconnectplugin.h>

class RunCommandPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit RunCommandPlugin(QObject *parent, const QVariantList &args);
    ~RunCommandPlugin();

    void receivePacket(const NetworkPacket &np) override;
    void connected() override;

private:
    QMap<unsigned int, QProcess *> currentProcesses;
    void startCommand(const NetworkPacket &np);
    void sendConfig();
    void onProcessFinished(unsigned int id, int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessReadyReadState(unsigned int id, bool isErrorOutput);
    void sendOutput(unsigned int id, const QStringList &standard, const QStringList &error) const;
};
