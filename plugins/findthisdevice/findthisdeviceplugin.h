/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <core/kdeconnectplugin.h>

#ifdef Q_OS_WIN
#include <QDebug>
#include <Windows.h>
#define INFO_BUFFER_SIZE 32767
#else
#include <QFile>
#include <QStandardPaths>
#include <QUrl>
#endif

#define PACKET_TYPE_FINDMYPHONE_REQUEST QStringLiteral("kdeconnect.findmyphone.request")

class FindThisDevicePlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.findthisdevice")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    QString dbusPath() const override;
    void receivePacket(const NetworkPacket &np) override;
};

inline QString defaultSound()
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
