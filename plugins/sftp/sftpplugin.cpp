/**
 * Copyright 2014 Samoilenko Yuri<kinnalru@gmail.com>
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

#include "sftpplugin.h"

#include <QDBusConnection>
#include <QDir>

#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KNotification>
#include <KRun>
#include <KStandardDirs>
#include <KFilePlacesModel>
#include <kde_file.h>

#include "sftp_config.h"
#include "mounter.h"
#include <core/kdebugnamespace.h>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect-plugins") )

static const QSet<QString> fields_c = QSet<QString>() << "ip" << "port" << "user" << "port" << "path";

struct SftpPlugin::Pimpl
{
    Pimpl() : mounter(0) {}
  
    //Add KIO entry to Dolphin's Places
    KFilePlacesModel  placesModel;
    Mounter* mounter;
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_d(new Pimpl)
{ 
    addToDolphin();
    kDebug(debugArea()) << "Created device:" << device()->name();
}

SftpPlugin::~SftpPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
    removeFromDolphin();    
    unmount();
    kDebug(debugArea()) << "Destroyed device:" << device()->name();
}

void SftpPlugin::addToDolphin()
{
    removeFromDolphin();
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    m_d->placesModel.addPlace(device()->name(), kioUrl, "kdeconnect");
    kDebug(debugArea()) << "add to dolphin";
}

void SftpPlugin::removeFromDolphin()
{
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    QModelIndex index = m_d->placesModel.closestItem(kioUrl);
    while (index.row() != -1) {
        m_d->placesModel.removePlace(index);
        index = m_d->placesModel.closestItem(kioUrl);
    }
}

void SftpPlugin::connected()
{
    bool state = QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents);
    kDebug(debugArea()) << "Exposing DBUS interface: " << state;
}

void SftpPlugin::mount()
{
    kDebug(debugArea()) << "Mount device:" << device()->name();
    if (m_d->mounter) {
        return;
    }

    KConfigGroup cfg = SftpConfig::config()->group("main");
    
    const int idleTimeout = cfg.readEntry("idle", true)
        ? cfg.readEntry("idletimeout", 60) * 60 * 1000
        : 0;
    
    m_d->mounter = new Mounter(this, idleTimeout);
    connect(m_d->mounter, SIGNAL(mounted()), this, SLOT(onMounted()));
    connect(m_d->mounter, SIGNAL(unmounted(bool)), this, SLOT(onUnmounted(bool)));
    connect(m_d->mounter, SIGNAL(failed(QString)), this, SLOT(onFailed(QString)));
}

void SftpPlugin::unmount()
{
    if (m_d->mounter)
    {
        m_d->mounter->deleteLater();
        m_d->mounter = 0;
    }
}

bool SftpPlugin::mountAndWait()
{
    mount();
    return m_d->mounter->wait();
}

bool SftpPlugin::isMounted() const
{
    return m_d->mounter && m_d->mounter->isMounted();
}

bool SftpPlugin::startBrowsing()
{
    if (mountAndWait()) {
        //return new KRun(KUrl::fromLocalFile(mountPoint()), 0);
        return new KRun(KUrl::fromPathOrUrl("kdeconnect://"+device()->id()), 0);
    }
    return false;
}

bool SftpPlugin::receivePackage(const NetworkPackage& np)
{
    if (!(fields_c - np.body().keys().toSet()).isEmpty())
    {
        // package is invalid
        return false;
    }
    
    Q_EMIT packageReceived(np);

    return true;
}

QString SftpPlugin::mountPoint()
{
    const QString mountDir = KStandardDirs::locateLocal("appdata", "", true, KComponentData("kdeconnect", "kdeconnect"));
    return QDir(mountDir).absoluteFilePath(device()->id());
}

void SftpPlugin::onMounted()
{
    kDebug(debugArea()) << device()->name() << QString("Remote filesystem mounted at %1").arg(mountPoint());

    Q_EMIT mounted();
}

void SftpPlugin::onUnmounted(bool idleTimeout)
{
    if (idleTimeout) {
        kDebug(debugArea()) << device()->name() << "Remote filesystem unmounted by idle timeout";
    } else {
        kDebug(debugArea()) << device()->name() << "Remote filesystem unmounted";
    }

    unmount();
    
    Q_EMIT unmounted();
}

void SftpPlugin::onFailed(const QString& message)
{
    knotify(KNotification::Error
        , message
        , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
    );

    unmount();

    Q_EMIT unmounted();
}

void SftpPlugin::knotify(int type, const QString& text, const QPixmap& icon) const
{
    KNotification::event(KNotification::StandardEvent(type)
      , i18n("Device %1", device()->name()), text, icon, 0
      , KNotification::CloseOnTimeout);
}

