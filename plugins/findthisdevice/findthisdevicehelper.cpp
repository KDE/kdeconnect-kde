/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 * SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "findthisdevicehelper.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#define INFO_BUFFER_SIZE 32767
#else
#include <QFile>
#include <QStandardPaths>
#endif

#include <QDebug>
#include <QUrl>

QString FindThisDeviceHelper::defaultSound()
{
    QString dirPath;
    QUrl soundURL;
#ifdef Q_OS_WIN
    wchar_t infoBuf[INFO_BUFFER_SIZE];
    if (!GetWindowsDirectory(infoBuf, INFO_BUFFER_SIZE)) {
        qWarning() << "Error with getting the Windows Directory.";
    } else {
        dirPath = QString::fromStdWString(infoBuf) + QStringLiteral("/media");
        if (!dirPath.isEmpty()) {
            soundURL = QUrl::fromUserInput(QStringLiteral("Ring01.wav"), dirPath, QUrl::AssumeLocalFile);
        }
    }
#else
    const QStringList dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dataLocation : dataLocations) {
        dirPath = dataLocation + QStringLiteral("/sounds");
        soundURL = QUrl::fromUserInput(QStringLiteral("Oxygen-Im-Phone-Ring.ogg"), dirPath, QUrl::AssumeLocalFile);
        if ((soundURL.isLocalFile() && soundURL.isValid() && QFile::exists(soundURL.toLocalFile()))) {
            break;
        }
    }
#endif
    if (soundURL.isEmpty()) {
        qWarning() << "Could not find default ring tone.";
    }
    return soundURL.toLocalFile();
}
