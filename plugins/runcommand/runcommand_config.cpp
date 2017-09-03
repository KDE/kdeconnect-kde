/**
 * Copyright 2015 David Edmundson <davidedmundson@kde.org>
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

#include "runcommand_config.h"

#include <QStandardPaths>
#include <QTableView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDebug>
#include <QUuid>
#include <QJsonDocument>

#include <KLocalizedString>

#include <KPluginFactory>

K_PLUGIN_FACTORY(ShareConfigFactory, registerPlugin<RunCommandConfig>();)

RunCommandConfig::RunCommandConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_runcommand_config"))
{
    QTableView* table = new QTableView(this);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(table);
    setLayout(layout);

    m_entriesModel = new QStandardItemModel(this);
    table->setModel(m_entriesModel);

    m_entriesModel->setHorizontalHeaderLabels(QStringList() << i18n("Name") << i18n("Command"));

}

RunCommandConfig::~RunCommandConfig()
{
}

void RunCommandConfig::defaults()
{
    KCModule::defaults();
    m_entriesModel->clear();

    Q_EMIT changed(true);
}

void RunCommandConfig::load()
{
    KCModule::load();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(config()->get<QByteArray>(QStringLiteral("commands"), "{}"));
    QJsonObject jsonConfig = jsonDocument.object();
    const QStringList keys = jsonConfig.keys();
    for (const QString& key : keys) {
        const QJsonObject entry = jsonConfig[key].toObject();
        const QString name = entry[QStringLiteral("name")].toString();
        const QString command = entry[QStringLiteral("command")].toString();

        QStandardItem* newName = new QStandardItem(name);
        newName->setEditable(true);
        newName->setData(key);
        QStandardItem* newCommand = new QStandardItem(command);
        newName->setEditable(true);

        m_entriesModel->appendRow(QList<QStandardItem*>() << newName << newCommand);
    }

    m_entriesModel->sort(0);

    insertEmptyRow();
    connect(m_entriesModel, &QAbstractItemModel::dataChanged, this, &RunCommandConfig::onDataChanged);

    Q_EMIT changed(false);
}

void RunCommandConfig::save()
{
    QJsonObject jsonConfig;
    for (int i=0;i < m_entriesModel->rowCount(); i++) {
        QString key = m_entriesModel->item(i, 0)->data().toString();
        const QString name = m_entriesModel->item(i, 0)->text();
        const QString command = m_entriesModel->item(i, 1)->text();

        if (name.isEmpty() || command.isEmpty()) {
            continue;
        }

        if (key.isEmpty()) {
            key = QUuid::createUuid().toString();
        }
        QJsonObject entry;
        entry[QStringLiteral("name")] = name;
        entry[QStringLiteral("command")] = command;
        jsonConfig[key] = entry;
    }
    QJsonDocument document;
    document.setObject(jsonConfig);
    config()->set(QStringLiteral("commands"), document.toJson(QJsonDocument::Compact));

    KCModule::save();

    Q_EMIT changed(false);
}

void RunCommandConfig::insertEmptyRow()
{
    QStandardItem* newName = new QStandardItem;
    newName->setEditable(true);
    QStandardItem* newCommand = new QStandardItem;
    newName->setEditable(true);

    m_entriesModel->appendRow(QList<QStandardItem*>() << newName << newCommand);
}

void RunCommandConfig::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    changed(true);
    Q_UNUSED(topLeft);
    if (bottomRight.row() == m_entriesModel->rowCount() - 1) {
        //TODO check both entries are still empty
        insertEmptyRow();
    }
}


#include "runcommand_config.moc"
