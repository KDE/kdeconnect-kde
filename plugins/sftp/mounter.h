/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <KProcess>

#include <QTimer>

#include "sftpplugin.h"

class Mounter : public QObject
{
    Q_OBJECT
public:
    explicit Mounter(SftpPlugin *sftp);
    ~Mounter() override;

    bool wait();
    bool isMounted() const
    {
        return m_started;
    }
    void onPacketReceived(const NetworkPacket &np);

Q_SIGNALS:
    void mounted();
    void unmounted();
    void failed(const QString &message);

private Q_SLOTS:
    void onStarted();
    void onError(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onMountTimeout();
    void start();

private:
    void unmount(bool finished);

private:
    SftpPlugin *m_sftp;
    KProcess *m_proc;
    QTimer m_connectTimer;
    QString m_mountPoint;
    bool m_started;
};
