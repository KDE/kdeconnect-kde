/**
 * SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "openconfig.h"

#include <QDebug>

#if HAVE_KIO
#include <KIO/CommandLauncherJob>
#endif

void OpenConfig::setXdgActivationToken(const QString &token)
{
    m_currentToken = token;
}

void OpenConfig::openConfiguration(const QString &deviceId, const QString &pluginId)
{
    QStringList args;

    QString argument;

    if (!deviceId.isEmpty()) {
        args << QStringLiteral("--args");
        argument = deviceId;

        if (!pluginId.isEmpty()) {
            argument += QLatin1Char(':') + pluginId;
        }

        args << argument;
    }

#if HAVE_KIO
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kdeconnect-settings"), args);
    job->setDesktopName(QStringLiteral("org.kde.kdeconnect-settings"));
    job->setStartupId(m_currentToken.toUtf8());
    job->start();
#else
    QProcess::startDetached(QStringLiteral("kdeconnect-settings"), args);
#endif

    m_currentToken = QString();
}
