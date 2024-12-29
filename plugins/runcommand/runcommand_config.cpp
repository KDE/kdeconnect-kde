/**
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "runcommand_config.h"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTableView>
#include <QUuid>

#include <KLocalizedString>
#include <KPluginFactory>

#include <dbushelper.h>

K_PLUGIN_CLASS(RunCommandConfig)

RunCommandConfig::RunCommandConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    // The qdbus executable name is different on some systems
    QString qdbusExe = QStringLiteral("qdbus-qt5");
    if (QStandardPaths::findExecutable(qdbusExe).isEmpty()) {
        qdbusExe = QStringLiteral("qdbus");
    }

    QMenu *defaultMenu = new QMenu(widget());

#ifdef Q_OS_WIN
    addSuggestedCommand(defaultMenu, i18n("Schedule a shutdown"), QStringLiteral("shutdown /s /t 60"));
    addSuggestedCommand(defaultMenu, i18n("Shutdown now"), QStringLiteral("shutdown /s /t 0"));
    addSuggestedCommand(defaultMenu, i18n("Cancel last shutdown"), QStringLiteral("shutdown /a"));
    addSuggestedCommand(defaultMenu, i18n("Schedule a reboot"), QStringLiteral("shutdown /r /t 60"));
    addSuggestedCommand(defaultMenu, i18n("Suspend"), QStringLiteral("rundll32.exe powrprof.dll,SetSuspendState 0,1,0"));
    addSuggestedCommand(defaultMenu, i18n("Lock Screen"), QStringLiteral("rundll32.exe user32.dll,LockWorkStation"));
    addSuggestedCommand(
        defaultMenu,
        i18n("Say Hello"),
        QStringLiteral("PowerShell -Command Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('hello');"));
#else
    addSuggestedCommand(defaultMenu, i18n("Shutdown"), QStringLiteral("systemctl poweroff"));
    addSuggestedCommand(defaultMenu, i18n("Reboot"), QStringLiteral("systemctl reboot"));
    addSuggestedCommand(defaultMenu, i18n("Suspend"), QStringLiteral("systemctl suspend"));
    addSuggestedCommand(
        defaultMenu,
        i18n("Maximum Brightness"),
        QStringLiteral("%0 org.kde.Solid.PowerManagement /org/kde/Solid/PowerManagement/Actions/BrightnessControl "
                       "org.kde.Solid.PowerManagement.Actions.BrightnessControl.setBrightness `%0 org.kde.Solid.PowerManagement "
                       "/org/kde/Solid/PowerManagement/Actions/BrightnessControl org.kde.Solid.PowerManagement.Actions.BrightnessControl.brightnessMax`")
            .arg(qdbusExe));
    addSuggestedCommand(defaultMenu, i18n("Lock Screen"), QStringLiteral("loginctl lock-session"));
    addSuggestedCommand(defaultMenu, i18n("Unlock Screen"), QStringLiteral("loginctl unlock-session"));
    addSuggestedCommand(defaultMenu, i18n("Close All Vaults"), QStringLiteral("%0 org.kde.kded5 /modules/plasmavault closeAllVaults").arg(qdbusExe));
    addSuggestedCommand(defaultMenu,
                        i18n("Forcefully Close All Vaults"),
                        QStringLiteral("%0 org.kde.kded5 /modules/plasmavault forceCloseAllVaults").arg(qdbusExe));
#endif

    QTableView *table = new QTableView(widget());
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    QPushButton *button = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Sample commands"), widget());
    button->setMenu(defaultMenu);

    QHBoxLayout *importExportLayout = new QHBoxLayout();
    QPushButton *exportButton = new QPushButton(i18n("Export"), widget());
    importExportLayout->addWidget(exportButton);
    connect(exportButton, &QPushButton::clicked, this, &RunCommandConfig::exportCommands);
    QPushButton *importButton = new QPushButton(i18n("Import"), widget());
    importExportLayout->addWidget(importButton);
    connect(importButton, &QPushButton::clicked, this, &RunCommandConfig::importCommands);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(table);
    layout->addLayout(importExportLayout);
    layout->addWidget(button);
    widget()->setLayout(layout);

    m_entriesModel = new QStandardItemModel(this);
    table->setModel(m_entriesModel);

    m_entriesModel->setHorizontalHeaderLabels(QStringList{i18n("Name"), i18n("Command")});
}

void RunCommandConfig::exportCommands()
{
    QString filePath = QFileDialog::getSaveFileName(widget(), i18n("Export Commands"), QDir::homePath(), QStringLiteral("JSON (*.json)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Could not write to file:" << filePath;
        return;
    }

    QJsonArray jsonArray;
    for (int i = 0; i < m_entriesModel->rowCount(); i++) {
        QJsonObject jsonObj;
        jsonObj[QStringLiteral("name")] = m_entriesModel->index(i, 0).data().toString();
        jsonObj[QStringLiteral("command")] = m_entriesModel->index(i, 1).data().toString();
        jsonArray.append(jsonObj);
    }

    QJsonDocument jsonDocument(jsonArray);
    file.write(jsonDocument.toJson());
    file.close();
}

void RunCommandConfig::importCommands()
{
    QString filePath = QFileDialog::getOpenFileName(widget(), i18n("Import Commands"), QDir::homePath(), QStringLiteral("JSON (*.json)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Could not read file:" << filePath;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull() || !jsonDoc.isArray()) {
        qWarning() << "Invalid JSON format.";
        return;
    }

    // Clear the current command list
    m_entriesModel->removeRows(0, m_entriesModel->rowCount());

    // Populate the model with the imported commands
    QJsonArray jsonArray = jsonDoc.array();
    for (const QJsonValue &jsonValue : jsonArray) {
        QJsonObject jsonObj = jsonValue.toObject();
        QString name = jsonObj.value(QStringLiteral("name")).toString();
        QString command = jsonObj.value(QStringLiteral("command")).toString();
        insertRow(m_entriesModel->rowCount(), name, command);
    }

    markAsChanged();
}

void RunCommandConfig::addSuggestedCommand(QMenu *menu, const QString &name, const QString &command)
{
    auto action = new QAction(name);
    connect(action, &QAction::triggered, action, [this, name, command]() {
        insertRow(0, name, command);
        markAsChanged();
    });
    menu->addAction(action);
}

void RunCommandConfig::defaults()
{
    KCModule::defaults();
    m_entriesModel->removeRows(0, m_entriesModel->rowCount());

    markAsChanged();
}

void RunCommandConfig::load()
{
    KCModule::load();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(config()->getByteArray(QStringLiteral("commands"), "{}"));
    QJsonObject jsonConfig = jsonDocument.object();
    const QStringList keys = jsonConfig.keys();
    for (const QString &key : keys) {
        const QJsonObject entry = jsonConfig[key].toObject();
        const QString name = entry[QStringLiteral("name")].toString();
        const QString command = entry[QStringLiteral("command")].toString();

        QStandardItem *newName = new QStandardItem(name);
        newName->setEditable(true);
        newName->setData(key);
        QStandardItem *newCommand = new QStandardItem(command);
        newName->setEditable(true);

        m_entriesModel->appendRow(QList<QStandardItem *>() << newName << newCommand);
    }

    m_entriesModel->sort(0);

    insertEmptyRow();
    connect(m_entriesModel, &QAbstractItemModel::dataChanged, this, &RunCommandConfig::onDataChanged);
}

void RunCommandConfig::save()
{
    KCModule::save();
    QJsonObject jsonConfig;
    for (int i = 0; i < m_entriesModel->rowCount(); i++) {
        QString key = m_entriesModel->item(i, 0)->data().toString();
        const QString name = m_entriesModel->item(i, 0)->text();
        const QString command = m_entriesModel->item(i, 1)->text();

        if (name.isEmpty() || command.isEmpty()) {
            continue;
        }

        if (key.isEmpty()) {
            key = QUuid::createUuid().toString(QUuid::WithoutBraces);
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
}

void RunCommandConfig::insertEmptyRow()
{
    insertRow(m_entriesModel->rowCount(), {}, {});
}

void RunCommandConfig::insertRow(int i, const QString &name, const QString &command)
{
    QStandardItem *newName = new QStandardItem(name);
    newName->setEditable(true);
    QStandardItem *newCommand = new QStandardItem(command);
    newName->setEditable(true);

    m_entriesModel->insertRow(i, QList<QStandardItem *>() << newName << newCommand);
}

void RunCommandConfig::onDataChanged(const QModelIndex & /*topLeft*/, const QModelIndex &bottomRight)
{
    markAsChanged();
    if (bottomRight.row() == m_entriesModel->rowCount() - 1) {
        // TODO check both entries are still empty
        insertEmptyRow();
    }
}

#include "moc_runcommand_config.cpp"
#include "runcommand_config.moc"
