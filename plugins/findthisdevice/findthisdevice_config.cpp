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

#include "findthisdevice_config.h"

#include "ui_findthisdevice_config.h"
// Phonon
#include <phonon/mediaobject.h>
// KF
#include <KLocalizedString>
#include <KPluginFactory>
// Qt
#include <QStandardPaths>


K_PLUGIN_FACTORY(FindThisDeviceConfigFactory, registerPlugin<FindThisDeviceConfig>();)


namespace {
namespace Strings {
inline QString defaultSound() { return QStringLiteral("Oxygen-Im-Phone-Ring.ogg"); }
}
}

FindThisDeviceConfig::FindThisDeviceConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_findthisdevice_config"))
    , m_ui(new Ui::FindThisDeviceConfigUi())
{
    m_ui->setupUi(this);

    const QStringList soundDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                            QStringLiteral("sounds"),
                                                            QStandardPaths::LocateDirectory);
    if (!soundDirs.isEmpty()) {
        m_ui->soundFileRequester->setStartDir(QUrl::fromLocalFile(soundDirs.last()));
    }

    connect(m_ui->playSoundButton, &QToolButton::clicked,
            this, &FindThisDeviceConfig::playSound);
    connect(m_ui->soundFileRequester, &KUrlRequester::textChanged,
            this, QOverload<>::of(&FindThisDeviceConfig::changed));
}

FindThisDeviceConfig::~FindThisDeviceConfig()
{
    delete m_ui;
}


void FindThisDeviceConfig::defaults()
{
    KCModule::defaults();

    m_ui->soundFileRequester->setText(Strings::defaultSound());

    Q_EMIT changed(true);
}

void FindThisDeviceConfig::load()
{
    KCModule::load();

    const QString ringTone = config()->get<QString>(QStringLiteral("ringtone"), Strings::defaultSound());
    m_ui->soundFileRequester->setText(ringTone);

    Q_EMIT changed(false);
}

void FindThisDeviceConfig::save()
{
    config()->set(QStringLiteral("ringtone"), m_ui->soundFileRequester->text());

    KCModule::save();

    Q_EMIT changed(false);
}

void FindThisDeviceConfig::playSound()
{
    const QString soundFilename = m_ui->soundFileRequester->text();

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

    Phonon::MediaObject *media = Phonon::createPlayer(Phonon::NotificationCategory, soundURL);
    media->play();
    connect(media, SIGNAL(finished()), media, SLOT(deleteLater()));
}


#include "findthisdevice_config.moc"
