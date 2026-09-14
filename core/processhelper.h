/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QProcess>
#include <QStringList>

namespace ProcessHelper
{
inline bool startDetached(const QString &program, const QStringList &arguments = {})
{
#ifdef Q_OS_UNIX
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setUnixProcessParameters(QProcess::UnixProcessFlag::CloseFileDescriptors);
    return process.startDetached();
#else
    return QProcess::startDetached(program, arguments);
#endif
}
}
