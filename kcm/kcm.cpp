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

#include <KDebug>
#include <kpluginfactory.h>
#include <kstandarddirs.h>

K_PLUGIN_FACTORY(KdeConnectKcmFactory, registerPlugin<KdeConnectKcm>();)
K_EXPORT_PLUGIN(KdeConnectKcmFactory("kdeconnect-kcm", "kdeconnect-kcm"))

KdeConnectKcm::KdeConnectKcm(QWidget *parent, const QVariantList&)
    : KCModule(KdeConnectKcmFactory::componentData(), parent)
    , kcmUi(new Ui::KdeConnectKcmUi())
    , pairedDevicesList(new DevicesModel(this))
    , config(KSharedConfig::openConfig("kdeconnectrc"))
{
    kcmUi->setupUi(this);

    kcmUi->deviceList->setIconSize(QSize(32,32));
    kcmUi->deviceList->setModel(pairedDevicesList);

    kcmUi->deviceInfo->setVisible(false);

    connect(kcmUi->deviceList, SIGNAL(pressed(QModelIndex)), this, SLOT(deviceSelected(QModelIndex)));
    connect(kcmUi->ping_button, SIGNAL(pressed()), this, SLOT(sendPing()));
    connect(kcmUi->trust_checkbox,SIGNAL(toggled(bool)), this, SLOT(trustedStateChanged(bool)));
}

KdeConnectKcm::~KdeConnectKcm()
{

}

void KdeConnectKcm::deviceSelected(const QModelIndex& current)
{
    bool valid = current.isValid();
    kcmUi->deviceInfo->setVisible(valid);
    if (!valid) return;
    selectedIndex = current;
    bool paired = pairedDevicesList->getDevice(current)->paired();
    kcmUi->trust_checkbox->setChecked(paired);
}

void KdeConnectKcm::trustedStateChanged(bool b)
{
    if (!selectedIndex.isValid()) return;
    DeviceDbusInterface* device = pairedDevicesList->getDevice(selectedIndex);
    device->setPair(b);
    pairedDevicesList->deviceStatusChanged(device->id());
}


void KdeConnectKcm::sendPing()
{
    if (!selectedIndex.isValid()) return;
    pairedDevicesList->getDevice(selectedIndex)->sendPing();
}


