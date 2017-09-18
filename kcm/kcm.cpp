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

KdeConnectKcm::KdeConnectKcm(QWidget* parent, const QVariantList&)
    : KCModule(KAboutData::pluginData(QStringLiteral("kdeconnect-kcm")), parent)
    , kcmUi(new Ui::KdeConnectKcmUi())
    , daemon(new DaemonDbusInterface(this))
    , devicesModel(new DevicesModel(this))
    , currentDevice(nullptr)
{
    KAboutData* about = new KAboutData(QStringLiteral("kdeconnect-kcm"),
                                       i18n("KDE Connect Settings"),
                                       QStringLiteral(KDECONNECT_VERSION_STRING),
                                       i18n("KDE Connect Settings module"),
                                       KAboutLicense::KAboutLicense::GPL_V2,
                                       i18n("(C) 2015 Albert Vaca Cintora"),
                                       QString(),
                                       QStringLiteral("https://community.kde.org/KDEConnect")
    );
    about->addAuthor(i18n("Albert Vaca Cintora"));
    setAboutData(about);

    kcmUi->setupUi(this);

    sortProxyModel = new DevicesSortProxyModel(devicesModel);

    kcmUi->deviceList->setModel(sortProxyModel);

    kcmUi->deviceInfo->setVisible(false);
    kcmUi->progressBar->setVisible(false);
    kcmUi->messages->setVisible(false);

    //Workaround: If we set this directly (or if we set it in the .ui file), the layout breaks
    kcmUi->noDeviceLinks->setWordWrap(false);
    QTimer::singleShot(0, [this] { kcmUi->noDeviceLinks->setWordWrap(true); });

    setWhenAvailable(daemon->announcedName(), [this](const QString& announcedName) {
        kcmUi->rename_label->setText(announcedName);
        kcmUi->rename_edit->setText(announcedName);
    }, this);
    connect(daemon, SIGNAL(announcedNameChanged(QString)),
            kcmUi->rename_edit, SLOT(setText(QString)));
    connect(daemon, SIGNAL(announcedNameChanged(QString)),
            kcmUi->rename_label, SLOT(setText(QString)));
    setRenameMode(false);

    setButtons(KCModule::Help | KCModule::NoAdditionalButton);

    connect(devicesModel, &QAbstractItemModel::dataChanged,
            this, &KdeConnectKcm::resetSelection);
    connect(kcmUi->deviceList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &KdeConnectKcm::deviceSelected);
    connect(kcmUi->accept_button, &QAbstractButton::clicked,
            this, &KdeConnectKcm::acceptPairing);
    connect(kcmUi->reject_button, &QAbstractButton::clicked,
            this, &KdeConnectKcm::rejectPairing);
    connect(kcmUi->pair_button, &QAbstractButton::clicked,
            this, &KdeConnectKcm::requestPair);
    connect(kcmUi->unpair_button, &QAbstractButton::clicked,
            this, &KdeConnectKcm::unpair);
    connect(kcmUi->ping_button, &QAbstractButton::clicked,
            this, &KdeConnectKcm::sendPing);
    connect(kcmUi->refresh_button,&QAbstractButton::clicked,
            this, &KdeConnectKcm::refresh);
    connect(kcmUi->rename_edit,&QLineEdit::returnPressed,
            this, &KdeConnectKcm::renameDone);
    connect(kcmUi->renameDone_button,&QAbstractButton::clicked,
            this, &KdeConnectKcm::renameDone);
    connect(kcmUi->renameShow_button,&QAbstractButton::clicked,
            this, &KdeConnectKcm::renameShow);

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
    resetDeviceView();

    connect(currentDevice, SIGNAL(pluginsChanged()), this, SLOT(resetCurrentDevice()));
    connect(currentDevice, SIGNAL(trustedChanged(bool)), this, SLOT(trustedChanged(bool)));
    connect(currentDevice, SIGNAL(pairingError(QString)), this, SLOT(pairingFailed(QString)));
    connect(currentDevice, &DeviceDbusInterface::hasPairingRequestsChangedProxy, this, &KdeConnectKcm::currentDevicePairingChanged);
}

void KdeConnectKcm::currentDevicePairingChanged(bool pairing)
{
    if (pairing) {
        setCurrentDeviceTrusted(RequestedByPeer);
    } else {
        setWhenAvailable(currentDevice->isTrusted(), [this](bool trusted) {
            setCurrentDeviceTrusted(trusted ? Trusted : NotTrusted);
        }, this);
    }
}

void KdeConnectKcm::resetCurrentDevice()
{
    const QStringList supportedPluginNames = currentDevice->supportedPlugins();

    if (m_oldSupportedPluginNames != supportedPluginNames) {
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
    setWhenAvailable(currentDevice->isTrusted(), [this](bool trusted) {
        if (trusted)
            setCurrentDeviceTrusted(Trusted);
        else
            setWhenAvailable(currentDevice->hasPairingRequests(), [this](bool haspr) {
                setCurrentDeviceTrusted(haspr ? RequestedByPeer : NotTrusted);
            }, this);
    }, this);

    const QList<KPluginInfo> pluginInfo = KPluginInfo::fromMetaData(KPluginLoader::findPlugins(QStringLiteral("kdeconnect/")));
    QList<KPluginInfo> availablePluginInfo;

    m_oldSupportedPluginNames = currentDevice->supportedPlugins();
    for (auto it = pluginInfo.cbegin(), itEnd = pluginInfo.cend(); it!=itEnd; ++it) {
        if (m_oldSupportedPluginNames.contains(it->pluginName())) {
            availablePluginInfo.append(*it);
        }
    }

    KSharedConfigPtr deviceConfig = KSharedConfig::openConfig(currentDevice->pluginsConfigFile());
    kcmUi->pluginSelector->addPlugins(availablePluginInfo, KPluginSelector::ReadConfigFile, i18n("Available plugins"), QString(), deviceConfig);
    connect(kcmUi->pluginSelector, &KPluginSelector::changed, this, &KdeConnectKcm::pluginsConfigChanged);

}

void KdeConnectKcm::requestPair()
{
    if (!currentDevice) {
        return;
    }

    kcmUi->messages->hide();

    setCurrentDeviceTrusted(Requested);

    currentDevice->requestPair();

}

void KdeConnectKcm::unpair()
{
    if (!currentDevice) {
        return;
    }

    setCurrentDeviceTrusted(NotTrusted);
    currentDevice->unpair();
}

void KdeConnectKcm::acceptPairing()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->acceptPairing();
}

void KdeConnectKcm::rejectPairing()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->rejectPairing();
}

void KdeConnectKcm::pairingFailed(const QString& error)
{
    if (sender() != currentDevice) return;

    setCurrentDeviceTrusted(NotTrusted);

    kcmUi->messages->setText(i18n("Error trying to pair: %1",error));
    kcmUi->messages->animatedShow();
}

void KdeConnectKcm::trustedChanged(bool trusted)
{
    DeviceDbusInterface* senderDevice = (DeviceDbusInterface*) sender();
    if (senderDevice == currentDevice)
        setCurrentDeviceTrusted(trusted ? Trusted : NotTrusted);
}

void KdeConnectKcm::setCurrentDeviceTrusted(KdeConnectKcm::TrustStatus trusted)
{
    kcmUi->accept_button->setVisible(trusted == RequestedByPeer);
    kcmUi->reject_button->setVisible(trusted == RequestedByPeer);
    kcmUi->pair_button->setVisible(trusted == NotTrusted);
    kcmUi->unpair_button->setVisible(trusted == Trusted);
    kcmUi->progressBar->setVisible(trusted == Requested);
    kcmUi->ping_button->setVisible(trusted == Trusted);
    switch (trusted) {
        case Trusted:
            kcmUi->status_label->setText(i18n("(paired)"));
            break;
        case NotTrusted:
            kcmUi->status_label->setText(i18n("(not paired)"));
            break;
        case RequestedByPeer:
            kcmUi->status_label->setText(i18n("(incoming pair request)"));
            break;
        case Requested:
            kcmUi->status_label->setText(i18n("(pairing requested)"));
            break;
    }
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
    currentDevice->pluginCall(QStringLiteral("ping"), QStringLiteral("sendPing"));
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
