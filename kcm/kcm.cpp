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

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStackedLayout>
#include <QListView>
#include <QDBusConnection>
#include <QDBusInterface>

#include <KServiceTypeTrader>
#include <KPluginInfo>
#include <KPluginMetaData>
#include <KPluginFactory>
#include <KAboutData>
#include <KLocalizedString>

#include "ui_kcm.h"
#include "interfaces/dbusinterfaces.h"
#include "interfaces/devicesmodel.h"
#include "devicessortproxymodel.h"
#include "kdeconnect-version.h"

K_PLUGIN_FACTORY(KdeConnectKcmFactory, registerPlugin<KdeConnectKcm>();)

static QString createId() { return QStringLiteral("kcm")+QString::number(QCoreApplication::applicationPid()); }

KdeConnectKcm::KdeConnectKcm(QWidget *parent, const QVariantList&)
    : KCModule(KAboutData::pluginData("kdeconnect-kcm"), parent)
    , kcmUi(new Ui::KdeConnectKcmUi())
    , daemon(new DaemonDbusInterface(this))
    , devicesModel(new DevicesModel(this))
    , currentDevice(nullptr)
{
    KAboutData *about = new KAboutData("kdeconnect-kcm",
                                       i18n("KDE Connect Settings"),
                                       QLatin1String(KDECONNECT_VERSION_STRING),
                                       i18n("KDE Connect Settings module"),
                                       KAboutLicense::KAboutLicense::GPL_V2,
                                       i18n("(C) 2015 Albert Vaca Cintora"),
                                       QString(),
                                       QLatin1String("https://community.kde.org/KDEConnect")
    );
    about->addAuthor(i18n("Albert Vaca Cintora"));
    setAboutData(about);

    kcmUi->setupUi(this);

    kcmUi->deviceList->setIconSize(QSize(32,32));

    sortProxyModel = new DevicesSortProxyModel(devicesModel);

    kcmUi->deviceList->setModel(sortProxyModel);

    kcmUi->deviceInfo->setVisible(false);
    kcmUi->progressBar->setVisible(false);
    kcmUi->messages->setVisible(false);

    //Workaround: If we set this directly (or if we set it in the .ui file), the layout breaks
    kcmUi->noDeviceLinks->setWordWrap(false);
    QTimer::singleShot(0, [this] { kcmUi->noDeviceLinks->setWordWrap(true); });

    kcmUi->rename_label->setText(daemon->announcedName());
    kcmUi->rename_edit->setText(daemon->announcedName());
    setRenameMode(false);

    setButtons(KCModule::NoAdditionalButton);

    connect(devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(resetSelection()));
    connect(kcmUi->deviceList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(deviceSelected(QModelIndex)));
    connect(kcmUi->pair_button, SIGNAL(clicked()),
            this, SLOT(requestPair()));
    connect(kcmUi->unpair_button, SIGNAL(clicked()),
            this, SLOT(unpair()));
    connect(kcmUi->ping_button, SIGNAL(clicked()),
            this, SLOT(sendPing()));
    connect(kcmUi->refresh_button,SIGNAL(clicked()),
            this, SLOT(refresh()));
    connect(kcmUi->rename_edit,SIGNAL(returnPressed()),
            this, SLOT(renameDone()));
    connect(kcmUi->renameDone_button,SIGNAL(clicked()),
            this, SLOT(renameDone()));
    connect(kcmUi->renameShow_button,SIGNAL(clicked()),
            this, SLOT(renameShow()));

    daemon->acquireDiscoveryMode(createId());
}

void KdeConnectKcm::renameShow()
{
    setRenameMode(true);
}

void KdeConnectKcm::renameDone()
{
    QString newName = kcmUi->rename_edit->text();
    if (newName.isEmpty()) {
        //Rollback changes
        kcmUi->rename_edit->setText(kcmUi->rename_label->text());
    } else {
        kcmUi->rename_label->setText(newName);
        daemon->setAnnouncedName(newName);
    }
    setRenameMode(false);
}

void KdeConnectKcm::setRenameMode(bool b) {
    kcmUi->renameDone_button->setVisible(b);
    kcmUi->rename_edit->setVisible(b);
    kcmUi->renameShow_button->setVisible(!b);
    kcmUi->rename_label->setVisible(!b);
}

KdeConnectKcm::~KdeConnectKcm()
{
    daemon->releaseDiscoveryMode(createId());
    delete kcmUi;
}

void KdeConnectKcm::refresh()
{
    daemon->acquireDiscoveryMode(createId());
    daemon->forceOnNetworkChange();
}

void KdeConnectKcm::resetSelection()
{
    if (!currentDevice) {
        return;
    }
    kcmUi->deviceList->selectionModel()->setCurrentIndex(sortProxyModel->mapFromSource(currentIndex), QItemSelectionModel::ClearAndSelect);
}

void KdeConnectKcm::deviceSelected(const QModelIndex& current)
{
    if (currentDevice) {
        disconnect(currentDevice, 0, this, 0);
    }

    //Store previous device config
    pluginsConfigChanged();

    if (!current.isValid()) {
        currentDevice = nullptr;
        kcmUi->deviceInfo->setVisible(false);
        return;
    }

    currentIndex = sortProxyModel->mapToSource(current);
    currentDevice = devicesModel->getDevice(currentIndex.row());

    kcmUi->noDevicePlaceholder->setVisible(false);
    bool valid = (currentDevice != nullptr && currentDevice->isValid());
    kcmUi->deviceInfo->setVisible(valid);
    if (!valid) {
        return;
    }

    kcmUi->messages->setVisible(false);
    if (currentDevice->pairRequested()) {
        kcmUi->progressBar->setVisible(true);
        kcmUi->unpair_button->setVisible(false);
        kcmUi->pair_button->setVisible(false);
        kcmUi->ping_button->setVisible(false);
    } else {
        kcmUi->progressBar->setVisible(false);
        if (currentDevice->isPaired()) {
            kcmUi->unpair_button->setVisible(true);
            kcmUi->pair_button->setVisible(false);
            kcmUi->ping_button->setVisible(true);
        } else {
            kcmUi->unpair_button->setVisible(false);
            kcmUi->pair_button->setVisible(true);
            kcmUi->ping_button->setVisible(false);
        }
    }
    resetDeviceView();

    connect(currentDevice, SIGNAL(pluginsChanged()), this, SLOT(resetCurrentDevice()));
    connect(currentDevice, SIGNAL(pairingChanged(bool)), this, SLOT(pairingChanged(bool)));
    connect(currentDevice, SIGNAL(pairingFailed(QString)), this, SLOT(pairingFailed(QString)));
}

void KdeConnectKcm::resetCurrentDevice()
{
    const QStringList unsupportedPluginNames = currentDevice->unsupportedPlugins();

    if (m_oldUnsupportedPluginNames != unsupportedPluginNames) {
        resetDeviceView();
    }
}

void KdeConnectKcm::resetDeviceView()
{
    //KPluginSelector has no way to remove a list of plugins and load another, so we need to destroy and recreate it each time
    delete kcmUi->pluginSelector;
    kcmUi->pluginSelector = new KPluginSelector(this);
    kcmUi->deviceInfo_layout->addWidget(kcmUi->pluginSelector);

    kcmUi->pluginSelector->setConfigurationArguments(QStringList(currentDevice->id()));

    kcmUi->name_label->setText(currentDevice->name());
    kcmUi->status_label->setText(currentDevice->isPaired()? i18n("(paired)") : i18n("(unpaired)"));

    const QList<KPluginInfo> pluginInfo = KPluginInfo::fromMetaData(KPluginLoader::findPlugins("kdeconnect/"));
    QList<KPluginInfo> availablePluginInfo;
    QList<KPluginInfo> unsupportedPluginInfo;

    m_oldUnsupportedPluginNames = currentDevice->unsupportedPlugins();
    for (auto it = pluginInfo.cbegin(), itEnd = pluginInfo.cend(); it!=itEnd; ++it) {
        if (m_oldUnsupportedPluginNames.contains(it->pluginName())) {
            unsupportedPluginInfo.append(*it);
        } else {
            availablePluginInfo.append(*it);
        }
    }

    KSharedConfigPtr deviceConfig = KSharedConfig::openConfig(currentDevice->pluginsConfigFile());
    kcmUi->pluginSelector->addPlugins(availablePluginInfo, KPluginSelector::ReadConfigFile, i18n("Available plugins"), QString(), deviceConfig);
    kcmUi->pluginSelector->addPlugins(unsupportedPluginInfo, KPluginSelector::ReadConfigFile, i18n("Plugins unsupported by the device"), QString(), deviceConfig);
    connect(kcmUi->pluginSelector, SIGNAL(changed(bool)), this, SLOT(pluginsConfigChanged()));

}

void KdeConnectKcm::requestPair()
{
    if (!currentDevice) {
        return;
    }

    kcmUi->messages->hide();

    kcmUi->pair_button->setVisible(false);
    kcmUi->progressBar->setVisible(true);

    currentDevice->requestPair();

}

void KdeConnectKcm::unpair()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->unpair();
}

void KdeConnectKcm::pairingFailed(const QString& error)
{
    if (sender() != currentDevice) return;

    pairingChanged(false);

    kcmUi->messages->setText(i18n("Error trying to pair: %1",error));
    kcmUi->messages->animatedShow();
}

void KdeConnectKcm::pairingChanged(bool paired)
{
    DeviceDbusInterface* senderDevice = (DeviceDbusInterface*) sender();
    if (senderDevice != currentDevice) return;

    kcmUi->pair_button->setVisible(!paired);
    kcmUi->unpair_button->setVisible(paired);
    kcmUi->progressBar->setVisible(senderDevice->pairRequested());
    kcmUi->ping_button->setVisible(paired);
    kcmUi->status_label->setText(paired ? i18n("(paired)") : i18n("(unpaired)"));
}

void KdeConnectKcm::pluginsConfigChanged()
{
    //Store previous selection
    if (!currentDevice) return;

    DeviceDbusInterface* auxCurrentDevice = currentDevice;
    currentDevice = nullptr; //HACK to avoid infinite recursion (for some reason calling save on pluginselector emits changed)
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
    currentDevice->pluginCall("ping", "sendPing");
}

QSize KdeConnectKcm::sizeHint() const
{
    return QSize(890,550); //Golden ratio :D
}

QSize KdeConnectKcm::minimumSizeHint() const
{
    return QSize(500,300);
}

#include "kcm.moc"
#include "moc_kcm.cpp"
