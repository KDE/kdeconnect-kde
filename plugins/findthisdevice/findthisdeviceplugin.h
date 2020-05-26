/**
 * Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 * Copyright 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FINDTHISDEVICEPLUGIN_H
#define FINDTHISDEVICEPLUGIN_H

#include <core/kdeconnectplugin.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#define INFO_BUFFER_SIZE 32767
#else
#include <QStandardPaths>
#include <QFile>
#include <QUrl>
#endif
// Qt
#include "plugin_findthisdevice_debug.h"

#define PACKET_TYPE_FINDMYPHONE_REQUEST QStringLiteral("kdeconnect.findmyphone.request")

class FindThisDevicePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.findthisdevice")

public:
    explicit FindThisDevicePlugin(QObject* parent, const QVariantList& args);
    ~FindThisDevicePlugin() override;

    void connected() override {};
    QString dbusPath() const override;
    bool receivePacket(const NetworkPacket& np) override;
};

inline QString defaultSound()
{
    QString dirPath;
    QUrl soundURL;
#ifdef Q_OS_WIN
    wchar_t infoBuf[INFO_BUFFER_SIZE];
    if(!GetWindowsDirectory(infoBuf, INFO_BUFFER_SIZE)) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Error with getting the Windows Directory.";
    } else {
        dirPath = QString::fromStdWString(infoBuf) + QStringLiteral("/media");
        if (!dirPath.isEmpty()) {
            soundURL = QUrl::fromUserInput(QStringLiteral("Ring01.wav"),
                                            dirPath,
                                            QUrl::AssumeLocalFile);
        }
    }
#else
    const QStringList dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dataLocation : dataLocations) {
        dirPath = dataLocation + QStringLiteral("/sounds");
        soundURL = QUrl::fromUserInput(QStringLiteral("Oxygen-Im-Phone-Ring.ogg"),
                                        dirPath,
                                        QUrl::AssumeLocalFile);
        if ((soundURL.isLocalFile() && soundURL.isValid() && QFile::exists(soundURL.toLocalFile()))) {
            break;
        }
    }
#endif
    if (soundURL.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Could not find default ring tone.";
    }
    return soundURL.toLocalFile();
}

#endif //FINDTHISDEVICEPLUGIN_H
