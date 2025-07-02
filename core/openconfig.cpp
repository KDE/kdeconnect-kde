/**
 * SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "openconfig.h"

#include <QDebug>
#include <QProcess>

#include <KIO/CommandLauncherJob>

void OpenConfig::setXdgActivationToken(const QString &token)
{
    m_currentToken = token;
}

void OpenConfig::openConfiguration(const QString &deviceId, const QString &pluginId)
{
    QStringList args;

    QString argument;

    if (!deviceId.isEmpty()) {
        args << QStringLiteral("--device");
        args << deviceId;

        if (!pluginId.isEmpty()) {
            args << QStringLiteral("--plugin-config");
            args << pluginId;
        }
    }

#if defined(Q_OS_WIN)
    QProcess::startDetached(QStringLiteral("kdeconnect-app.exe"), args);
#elif defined(Q_OS_MAC)
    QProcess::startDetached(QCoreApplication::applicationDirPath() + QLatin1String("/kdeconnect-app"), args);
#else
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kdeconnect-app"), args);
    job->setDesktopName(QStringLiteral("org.kde.kdeconnect.app"));
    job->setStartupId(m_currentToken.toUtf8());
    job->start();

    m_currentToken = QString();
#endif
}

#include "moc_openconfig.cpp"
