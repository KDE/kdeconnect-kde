/**
 * Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "findthisdeviceplugin.h"

// Phonon
#include <phonon/mediaobject.h>
// KF
#include <KPluginFactory>
// Qt
#include <QDBusConnection>
#include <QStandardPaths>
#include <QFile>
#include <QUrl>


K_PLUGIN_FACTORY_WITH_JSON(KdeConnectPluginFactory, "kdeconnect_findthisdevice.json",
                           registerPlugin<FindThisDevicePlugin>();)

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_FINDTHISDEVICE, "kdeconnect.plugin.findthisdevice")

namespace {
namespace Strings {
inline QString defaultSound() { return QStringLiteral("Oxygen-Im-Phone-Ring.ogg"); }
}
}

FindThisDevicePlugin::FindThisDevicePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

FindThisDevicePlugin::~FindThisDevicePlugin() = default;

void FindThisDevicePlugin::connected()
{
}

bool FindThisDevicePlugin::receivePacket(const NetworkPacket& np)
{
    Q_UNUSED(np);

    const QString soundFilename = config()->get<QString>(QStringLiteral("ringtone"), Strings::defaultSound());

    QUrl soundURL;
    const auto dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dataLocation : dataLocations) {
        soundURL = QUrl::fromUserInput(soundFilename,
                                       dataLocation + QStringLiteral("/sounds"),
                                       QUrl::AssumeLocalFile);
        if (soundURL.isLocalFile()) {
            if (QFile::exists(soundURL.toLocalFile())) {
                break;
            }
        } else {
            if (soundURL.isValid()) {
                break;
            }
        }
        soundURL.clear();
    }
    if (soundURL.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Not playing sounds, could not find ring tone" << soundFilename;
        return true;
    }

    Phonon::MediaObject *media = Phonon::createPlayer(Phonon::NotificationCategory, soundURL);  // or CommunicationCategory?
    media->play();
    connect(media, &Phonon::MediaObject::finished, media, &QObject::deleteLater);

    // TODO: by-pass volume settings in case it is muted
    // TODO: ensure to use built-in loudspeakers

    return true;
}

QString FindThisDevicePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/findthisdevice";
}

#include "findthisdeviceplugin.moc"

