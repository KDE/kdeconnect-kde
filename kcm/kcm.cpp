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

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QStackedLayout>
#include <QtGui/QListView>

#include <KDebug>
#include <kpluginfactory.h>
#include <kstandarddirs.h>

K_PLUGIN_FACTORY(KdeConnectKcmFactory, registerPlugin<KdeConnectKcm>();)
K_EXPORT_PLUGIN(KdeConnectKcmFactory("kdeconnect-kcm", "kdeconnect-kcm"))

KdeConnectKcm::KdeConnectKcm(QWidget *parent, const QVariantList&)
    : KCModule(KdeConnectKcmFactory::componentData(), parent)
{
    m_ui = new Ui::KdeConnectKcmUi();
    m_ui->setupUi(this);

    m_model = new QStandardItemModel(this);
    //m_selectionModel = new QItemSelectionModel(m_model);
    //connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(currentChanged(QModelIndex,QModelIndex)));
    //m_selectionModel->setCurrentIndex(m_model->index(0), QItemSelectionModel::SelectCurrent);

    m_ui->deviceList->setIconSize(QSize(32,32));
    m_ui->deviceList->setModel(m_model);
    //m_ui->deviceList->setSelectionModel(m_selectionModel);

    connect(m_ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeButtonClicked()));
    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addButtonClicked()));
}

KdeConnectKcm::~KdeConnectKcm()
{

}

void KdeConnectKcm::addButtonClicked()
{

}

void KdeConnectKcm::removeButtonClicked()
{

}

void KdeConnectKcm::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{

}

#include "kcm.moc"
