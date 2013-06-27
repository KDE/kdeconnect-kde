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

#include <QDebug>
#include <QStandardItemModel>

#include "ui_wizard.h"

AddDeviceWizard::AddDeviceWizard(QWidget* parent)
    : QWizard(parent)
    , wizardUi(new Ui::Wizard())
    , dbusInterface(new DaemonDbusInterface(this))
    , discoveredDevicesList(new QStandardItemModel(this))
{

    wizardUi->setupUi(this);

    wizardUi->listView->setModel(discoveredDevicesList);

    connect(this,SIGNAL(currentIdChanged(int)),this,SLOT(pageChanged(int)));

    connect(dbusInterface, SIGNAL(deviceAdded(QString, QString)), this, SLOT(deviceDiscovered(QString,QString)));
    connect(dbusInterface, SIGNAL(deviceRemoved(QString)), this, SLOT(deviceLost(QString)));

}

void AddDeviceWizard::pageChanged(int id)
{
    qDebug() << id;
    //QWizardPage* p = page(id);
    if (id == 1) {
        //Show "scanning"
    }
}

void AddDeviceWizard::deviceDiscovered(QString id, QString name)
{
    QStandardItem* item = new QStandardItem(name);
    item->setData(id);

    discoveredDevicesList->appendRow(item);
}

void AddDeviceWizard::deviceLost(QString id)
{
    //discoveredDevicesList->removeRow();
}

void AddDeviceWizard::discoveryFinished(bool success)
{

}

AddDeviceWizard::~AddDeviceWizard()
{
    delete wizardUi;
    delete dbusInterface;
    delete discoveredDevicesList;
}
