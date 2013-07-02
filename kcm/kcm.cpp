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
    , m_dbusInterface(new DaemonDbusInterface(this))
    , pairedDevicesList(new DevicesModel(this))
    , addDeviceWizard(new AddDeviceWizard(this))
    , config(KSharedConfig::openConfig("kdeconnectrc"))
{
    config->group("devices").group("paired").group("123456").writeEntry("name","Ultra-fake device");
    config->group("devices").group("paired").group("987654").writeEntry("name","Ultra-fake device");

    pairedDevicesList->loadPaired();

    kcmUi->setupUi(this);

    kcmUi->deviceList->setIconSize(QSize(32,32));
    kcmUi->deviceList->setModel(pairedDevicesList);

    connect(kcmUi->addButton, SIGNAL(clicked(bool)), this, SLOT(addButtonClicked()));
    connect(kcmUi->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeButtonClicked()));

    connect(kcmUi->deviceList, SIGNAL(pressed(QModelIndex)), this, SLOT(deviceSelected(QModelIndex)));

    connect(addDeviceWizard,SIGNAL(deviceAdded(QString,QString)),this, SLOT(deviceAdded(QString,QString)));
}

KdeConnectKcm::~KdeConnectKcm()
{

}

void KdeConnectKcm::addButtonClicked()
{
    addDeviceWizard->show();
}

void KdeConnectKcm::removeButtonClicked()
{
    QModelIndex selectedIndex = kcmUi->deviceList->currentIndex();
    if (selectedIndex.isValid() && selectedIndex.row() >= 0 && selectedIndex.row() < pairedDevicesList->rowCount()) {
        QString id = pairedDevicesList->data(selectedIndex,DevicesModel::IdModelRole).toString();
        if (m_dbusInterface->unpairDevice(id)) {;
            pairedDevicesList->loadPaired();
        }
    }

}

void KdeConnectKcm::deviceAdded(QString id,QString name)
{
    if (m_dbusInterface->pairDevice(id)) {
        qDebug() << "Succesfully paired: " + id;
        pairedDevicesList->loadPaired();
    }

}

void KdeConnectKcm::deviceSelected(const QModelIndex& current)
{
    qDebug() << "PENIS";
    kcmUi->removeButton->setEnabled(current.isValid());
}


