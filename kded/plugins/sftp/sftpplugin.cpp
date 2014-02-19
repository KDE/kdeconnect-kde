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
#include "../../kdebugnamespace.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SftpPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_sftp", "kdeconnect_sftp") )

static const QSet<QString> fields_c = QSet<QString>() << "ip" << "port" << "user" << "port" << "path";

struct SftpPlugin::Pimpl
{
    Pimpl()
    {
        //Add KIO entry to Dolphin's Places
        placesModel = new KFilePlacesModel();
    }
  
    KFilePlacesModel*  placesModel;
    QPointer<Mounter> mounter;
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_d(new Pimpl)
{ 
    addToDolphin();
    kDebug(kdeconnect_kded()) << "Created device:" << device()->name();
}

SftpPlugin::~SftpPlugin()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
    removeFromDolphin();    
    unmount();
    kDebug(kdeconnect_kded()) << "Destroyed device:" << device()->name();
}

void SftpPlugin::addToDolphin()
{
    removeFromDolphin();
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    m_d->placesModel->addPlace(device()->name(), kioUrl, "kdeconnect");
    kDebug(kdeconnect_kded()) << "add to dolphin";
}

void SftpPlugin::removeFromDolphin()
{
    KUrl kioUrl("kdeconnect://"+device()->id()+"/");
    QModelIndex index = m_d->placesModel->closestItem(kioUrl);
    while (index.row() != -1) {
        m_d->placesModel->removePlace(index);
        index = m_d->placesModel->closestItem(kioUrl);
    }
}

void SftpPlugin::connected()
{
    kDebug(kdeconnect_kded()) << "Exposing DBUS interface: "
        << QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents);
}

void SftpPlugin::mount()
{
    kDebug(kdeconnect_kded()) << "Device:" << device()->name();
    if (m_d->mounter)
    {
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
    kDebug(kdeconnect_kded()) << "Device:" << device()->name();
    delete m_d->mounter.data();
}

bool SftpPlugin::mountAndWait()
{
    mount();
    return m_d->mounter->wait();
}

bool SftpPlugin::isMounted()
{
    return m_d->mounter && m_d->mounter->isMounted();
}

bool SftpPlugin::startBrowsing()
{
    if (mountAndWait())
    {
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
    const QString defaultMountDir = KStandardDirs::locateLocal("appdata", "", true, componentData());
    const QString mountDir = KConfig("kdeconnect/plugins/sftp").group("main").readEntry("mountpoint", defaultMountDir);
    return QDir(mountDir).absoluteFilePath(device()->id());
}

void SftpPlugin::onMounted()
{
    kDebug(kdeconnect_kded()) << device()->name() << QString("Remote filesystem mounted at %1").arg(mountPoint());

    KNotification* notification = new KNotification("mounted", KNotification::CloseOnTimeout, this);
    notification->setPixmap(KIconLoader::global()->loadIcon("drive-removable-media", KIconLoader::Desktop));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(i18n("Device %1").arg(device()->name()));
    notification->setText(i18n("Filesystem mounted at %1").arg(mountPoint()));
    notification->sendEvent();

    Q_EMIT mounted();
}

void SftpPlugin::onUnmounted(bool idleTimeout)
{
    if (idleTimeout)
    {
        kDebug(kdeconnect_kded()) << device()->name() << "Remote filesystem unmounted by idle timeout";
    }
    else
    {
        kDebug(kdeconnect_kded()) << device()->name() << "Remote filesystem unmounted";
    }

    KNotification* notification = new KNotification("unmounted", KNotification::CloseOnTimeout, this);
    notification->setPixmap(KIconLoader::global()->loadIcon("dialog-ok", KIconLoader::Desktop));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(i18n("Device %1").arg(device()->name()));
    notification->setText(i18n("Filesystem unmounted"));
    notification->sendEvent();
    
    m_d->mounter->deleteLater();
    m_d->mounter = 0;
    
    Q_EMIT unmounted();
}

void SftpPlugin::onFailed(const QString& message)
{
    knotify(KNotification::Error
        , message
        , KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Desktop)
    );
    m_d->mounter->deleteLater();
    m_d->mounter = 0;
    
    Q_EMIT unmounted();
}

void SftpPlugin::knotify(int type, const QString& text, const QPixmap& icon) const
{
    KNotification::event(KNotification::StandardEvent(type)
      , i18n("Device %1").arg(device()->name()), text, icon, 0
      , KNotification::CloseOnTimeout);
}

