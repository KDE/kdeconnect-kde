/*
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

#include "wizard.h"
#include "devicesmodel.h"

#include <QDebug>
#include <QStandardItemModel>

#include <kdebug.h>

#include "ui_wizard.h"

AddDeviceWizard::AddDeviceWizard(QWidget* parent)
    : QWizard(parent)
    , wizardUi(new Ui::Wizard())
    , dbusInterface(new DaemonDbusInterface(this))
    , discoveredDevicesList(new DevicesModel(this))
{

    wizardUi->setupUi(this);

    connect(wizardUi->listView,SIGNAL(activated(QModelIndex)),this,SLOT(deviceSelected(QModelIndex)));

    wizardUi->listView->setModel(discoveredDevicesList);

    connect(this,SIGNAL(currentIdChanged(int)),this,SLOT(pageChanged(int)));

    connect(this,SIGNAL(accepted()),this,SLOT(wizardFinished()));

    connect(dbusInterface, SIGNAL(deviceDiscovered(QString, QString)), this, SLOT(deviceDiscovered(QString,QString)));
    //connect(dbusInterface, SIGNAL(deviceLost(QString)), this, SLOT(deviceLost(QString)));

    dbusInterface->startDiscovery(123456789);

}

void AddDeviceWizard::wizardFinished()
{
    if (selectedIndex.row() > 0 && selectedIndex.row() < discoveredDevicesList->rowCount()) {
        QString name = discoveredDevicesList->data(selectedIndex,DevicesModel::NameModelRole).toString();
        QString id = discoveredDevicesList->data(selectedIndex,DevicesModel::IdModelRole).toString();
        emit deviceAdded(name,id);
    }
}


void AddDeviceWizard::pageChanged(int id)
{
    qDebug() << id;
    if (id == 2) {
        //TODO: Do the actual pairing in this page
    }
}

void AddDeviceWizard::deviceDiscovered(QString id, QString name)
{
    discoveredDevicesList->addDevice(id,name,DevicesModel::Visible);
}
/*
void AddDeviceWizard::deviceLost(QString id)
{
    //discoveredDevicesList->removeRow();
}
*/
void AddDeviceWizard::discoveryFinished(bool success)
{

}

void AddDeviceWizard::restart()
{
    selectedIndex = QModelIndex();
    QWizard::restart();
}


void AddDeviceWizard::show()
{
    restart();
    QWizard::show();
}

void AddDeviceWizard::deviceSelected(const QModelIndex& index)
{
    qDebug() << "Selected: " + index.row();
    selectedIndex = index;
    next();
}

AddDeviceWizard::~AddDeviceWizard()
{
    delete wizardUi;
    delete dbusInterface;
    delete discoveredDevicesList;
}
