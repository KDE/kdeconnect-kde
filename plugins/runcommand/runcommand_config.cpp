/**
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "runcommand_config.h"

#include <QTableView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QStandardItemModel>
#include <QDebug>
#include <QUuid>
#include <QJsonDocument>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KPluginFactory>

#include <dbushelper.h>

K_PLUGIN_FACTORY(ShareConfigFactory, registerPlugin<RunCommandConfig>();)


RunCommandConfig::RunCommandConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_runcommand_config"))
{
    // The qdbus executable name is different on some systems
    QString qdbusExe = QStringLiteral("qdbus-qt5");
    if (QStandardPaths::findExecutable(qdbusExe).isEmpty()) {
        qdbusExe = QStringLiteral("qdbus");
    }

    QMenu* defaultMenu = new QMenu(this);

#ifdef Q_OS_WIN
    addSuggestedCommand(defaultMenu, i18n("Schedule a shutdown"), QStringLiteral("shutdown /s /t 60"));
    addSuggestedCommand(defaultMenu, i18n("Shutdown now"), QStringLiteral("shutdown /s /t 0"));
    addSuggestedCommand(defaultMenu, i18n("Cancel last shutdown"), QStringLiteral("shutdown /a"));
    addSuggestedCommand(defaultMenu, i18n("Schedule a reboot"), QStringLiteral("shutdown /r /t 60"));
    addSuggestedCommand(defaultMenu, i18n("Suspend"), QStringLiteral("rundll32.exe powrprof.dll,SetSuspendState 0,1,0"));
    addSuggestedCommand(defaultMenu, i18n("Lock Screen"), QStringLiteral("rundll32.exe user32.dll,LockWorkStation"));
    addSuggestedCommand(defaultMenu, i18n("Say Hello"), QStringLiteral("PowerShell -Command \"Add-Type –AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('hello');\""));
#else
    addSuggestedCommand(defaultMenu, i18n("Shutdown"), QStringLiteral("systemctl poweroff"));
    addSuggestedCommand(defaultMenu, i18n("Reboot"), QStringLiteral("systemctl reboot"));
    addSuggestedCommand(defaultMenu, i18n("Suspend"), QStringLiteral("systemctl suspend"));
    addSuggestedCommand(defaultMenu, i18n("Maximum Brightness"), QStringLiteral("%0 org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.setBrightness `%0 org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.brightnessMax`").arg(qdbusExe));
    addSuggestedCommand(defaultMenu, i18n("Lock Screen"), QStringLiteral("loginctl lock-session"));
    addSuggestedCommand(defaultMenu, i18n("Unlock Screen"), QStringLiteral("loginctl unlock-session"));
    addSuggestedCommand(defaultMenu, i18n("Close All Vaults"), QStringLiteral("%0 org.kde.kded5 /modules/plasmavault closeAllVaults").arg(qdbusExe));
    addSuggestedCommand(defaultMenu, i18n("Forcefully Close All Vaults"), QStringLiteral("%0 org.kde.kded5 /modules/plasmavault forceCloseAllVaults").arg(qdbusExe));
#endif

    QTableView* table = new QTableView(this);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(table);
    QPushButton* button = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Sample commands"), this);
    button->setMenu(defaultMenu);
    layout->addWidget(button);

    setLayout(layout);

    m_entriesModel = new QStandardItemModel(this);
    table->setModel(m_entriesModel);

    m_entriesModel->setHorizontalHeaderLabels(QStringList() << i18n("Name") << i18n("Command"));

}

RunCommandConfig::~RunCommandConfig()
{
}

void RunCommandConfig::addSuggestedCommand(QMenu* menu, const QString &name, const QString &command)
{
    auto action = new QAction(name);
    connect(action, &QAction::triggered, action, [this, name, command]() {
        insertRow(0, name, command);
        Q_EMIT changed(true);
    });
    menu->addAction(action);
}

void RunCommandConfig::defaults()
{
    KCModule::defaults();
    m_entriesModel->removeRows(0,m_entriesModel->rowCount());

    Q_EMIT changed(true);
}

void RunCommandConfig::load()
{
    KCModule::load();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(config()->getByteArray(QStringLiteral("commands"), "{}"));
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
            DBusHelper::filterNonExportableCharacters(key);
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
    insertRow(m_entriesModel->rowCount(), {}, {});
}

void RunCommandConfig::insertRow(int i, const QString& name, const QString& command)
{
    QStandardItem* newName = new QStandardItem(name);
    newName->setEditable(true);
    QStandardItem* newCommand = new QStandardItem(command);
    newName->setEditable(true);

    m_entriesModel->insertRow(i, QList<QStandardItem*>() << newName << newCommand);
}

void RunCommandConfig::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_EMIT changed(true);
    Q_UNUSED(topLeft);
    if (bottomRight.row() == m_entriesModel->rowCount() - 1) {
        //TODO check both entries are still empty
        insertEmptyRow();
    }
}


#include "runcommand_config.moc"
