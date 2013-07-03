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

#ifndef WIZARD_H
#define WIZARD_H

#include <QWizard>
#include <QObject>

#include "dbusinterfaces.h"
#include "devicesmodel.h"

namespace Ui {
    class Wizard;
}

class QStandardItemModel;

class AddDeviceWizard : public QWizard
{
    Q_OBJECT

public:
    AddDeviceWizard(QWidget* parent);
    ~AddDeviceWizard();
    void show();
    void restart();

private Q_SLOTS:
    void pageChanged(int id);
    
    void deviceDiscovered(QString id, QString name);
    //void deviceLost(QString id);

    void deviceSelected(const QModelIndex& index);

    void wizardFinished();

Q_SIGNALS:
    void deviceAdded(QString id, QString name);

private:
    Ui::Wizard* wizardUi;
    DaemonDbusInterface* dbusInterface;
    DevicesModel* discoveredDevicesList;
    QModelIndex selectedIndex;
};

#endif // WIZARD_H
