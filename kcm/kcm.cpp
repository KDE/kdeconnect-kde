/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "kcm.h"
#include "ui_kcm.h"
#include "ui_wizard.h"
#include "dbusinterfaces.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QStackedLayout>
#include <QtGui/QListView>
#include <QDBusConnection>
#include <QDBusInterface>

#include "devicessortproxymodel.h"

#include <KServiceTypeTrader>
#include <KPluginInfo>
#include <KDebug>
#include <kpluginfactory.h>
#include <kstandarddirs.h>

K_PLUGIN_FACTORY(KdeConnectKcmFactory, registerPlugin<KdeConnectKcm>();)
K_EXPORT_PLUGIN(KdeConnectKcmFactory("kdeconnect-kcm", "kdeconnect-kcm"))

KdeConnectKcm::KdeConnectKcm(QWidget *parent, const QVariantList&)
    : KCModule(KdeConnectKcmFactory::componentData(), parent)
    , kcmUi(new Ui::KdeConnectKcmUi())
    , devicesModel(new DevicesModel(this))
    , currentDevice(0)
    //, config(KSharedConfig::openConfig("kdeconnectrc"))
{
    kcmUi->setupUi(this);

    kcmUi->deviceList->setIconSize(QSize(32,32));

    sortProxyModel = new DevicesSortProxyModel(devicesModel);

    kcmUi->deviceList->setModel(sortProxyModel);

    kcmUi->deviceInfo->setVisible(false);

    setButtons(KCModule::NoAdditionalButton);

    connect(devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(resetSelection()));
    connect(kcmUi->deviceList, SIGNAL(pressed(QModelIndex)),
            this, SLOT(deviceSelected(QModelIndex)));
    connect(kcmUi->ping_button, SIGNAL(pressed()),
            this, SLOT(sendPing()));
    connect(kcmUi->trust_checkbox, SIGNAL(toggled(bool)),
            this, SLOT(trustedStateChanged(bool)));



}

KdeConnectKcm::~KdeConnectKcm()
{

}

void KdeConnectKcm::resetSelection()
{
    kcmUi->deviceList->selectionModel()->setCurrentIndex(sortProxyModel->mapFromSource(currentIndex), QItemSelectionModel::ClearAndSelect);
}

void KdeConnectKcm::deviceSelected(const QModelIndex& current)
{

    //Store previous device config
    pluginsConfigChanged();

    if (!current.isValid()) {
        kcmUi->deviceInfo->setVisible(false);
        return;
    }

    currentIndex = sortProxyModel->mapToSource(current);
    currentDevice = devicesModel->getDevice(currentIndex);

    bool valid = (currentDevice != NULL && currentDevice->isValid());
    kcmUi->deviceInfo->setVisible(valid);
    if (!valid) {
        return;
    }

    //FIXME: KPluginSelector has no way to remove a list of plugins and load another, so we need to destroy and recreate it each time
    delete kcmUi->pluginSelector;
    kcmUi->pluginSelector = new KPluginSelector(this);
    kcmUi->verticalLayout_2->addWidget(kcmUi->pluginSelector);

    kcmUi->deviceName->setText(currentDevice->name());
    kcmUi->trust_checkbox->setChecked(currentDevice->paired());

    KService::List offers = KServiceTypeTrader::self()->query("KdeConnect/Plugin");
    QList<KPluginInfo> scriptinfos = KPluginInfo::fromServices(offers);

    QString path = KStandardDirs().resourceDirs("config").first()+"kdeconnect/";
    KSharedConfigPtr deviceConfig = KSharedConfig::openConfig(path + currentDevice->id());
    kcmUi->pluginSelector->addPlugins(scriptinfos, KPluginSelector::ReadConfigFile, "Plugins", QString(), deviceConfig);

    connect(kcmUi->pluginSelector, SIGNAL(changed(bool)),
            this, SLOT(pluginsConfigChanged()));
}

void KdeConnectKcm::trustedStateChanged(bool b)
{
    if (!currentDevice) return;
    QDBusPendingReply<void> pendingReply = currentDevice->setPair(b);
    pendingReply.waitForFinished();
    if (pendingReply.isValid()) {
        //If dbus was down, calling this would make kcm crash
        devicesModel->deviceStatusChanged(currentDevice->id());
    } else {
        //Revert checkbox
        disconnect(kcmUi->trust_checkbox, SIGNAL(toggled(bool)),
                   this, SLOT(trustedStateChanged(bool)));
        kcmUi->trust_checkbox->setCheckState(b? Qt::Unchecked : Qt::Checked);
        connect(kcmUi->trust_checkbox, SIGNAL(toggled(bool)),
                this, SLOT(trustedStateChanged(bool)));
    }
}

void KdeConnectKcm::pluginsConfigChanged()
{
    //Store previous selection
    if (!currentDevice) return;

    DeviceDbusInterface* auxCurrentDevice = currentDevice; //HACK to avoid infinite recursion (for some reason calling save on pluginselector emits changed)
    currentDevice = 0;
    kcmUi->pluginSelector->save();
    currentDevice = auxCurrentDevice;

    currentDevice->reloadPlugins();
}

void KdeConnectKcm::save()
{
    pluginsConfigChanged();
    KCModule::save();
}

void KdeConnectKcm::sendPing()
{
    if (!currentDevice) return;
    currentDevice->sendPing();
}

