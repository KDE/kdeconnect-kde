/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kcm.h"

#include <KColorSchemeManager>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <QMessageBox>
#include <QtWidgets/QListView>
#include <kcmutils_version.h>

#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <qassert.h>

#include "dbushelpers.h"
#include "dbusinterfaces.h"
#include "devicesmodel.h"
#include "devicessortproxymodel.h"
#include "kdeconnect-version.h"

K_PLUGIN_CLASS_WITH_JSON(KdeConnectKcm, "kcm_kdeconnect.json")

class QQuickWidgetPaleteChangeWatcher : public QObject
{
    using QObject::QObject;

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
            // We know that watched is a QQuickWidget
            QQuickWidget *w = static_cast<QQuickWidget *>(watched);
            w->setClearColor(w->palette().color(QPalette::Window));
        }
        return QObject::eventFilter(watched, event);
    }
};

void adjustListWidgetHeight(QListWidget *listWidget)
{
    int totalHeight = 0;
    for (int i = 0; i < listWidget->count(); ++i) {
        totalHeight += listWidget->sizeHintForRow(i);
    }
    totalHeight += 2 * listWidget->frameWidth();
    listWidget->setFixedHeight(totalHeight);
}

KdeConnectKcm::KdeConnectKcm(QObject *parent, const KPluginMetaData &md, const QVariantList &args)
    : KCModule(parent, md)
    , daemon(new DaemonDbusInterface(this))
    , devicesModel(new DevicesModel(this))
    , currentDevice(nullptr)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    // Ensure we have a suitable color theme set for light/dark mode. KColorSchemeManager implicitly applies
    // a suitable default theme.
    KColorSchemeManager::instance();
    // Force breeze style to ensure coloring works consistently in dark mode. Specifically tab colors have
    // troubles on windows.
    QApplication::setStyle(QStringLiteral("breeze"));
    // Force breeze icon theme to ensure we can correctly adapt icons to color changes WRT dark/light mode.
    // Without this we may end up with hicolor and fail to support icon recoloring.
    QIcon::setThemeName(QStringLiteral("breeze"));
#else
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    kcmUi.setupUi(widget());

    sortProxyModel = new DevicesSortProxyModel(devicesModel);

    kcmUi.list_quick_widget->setMinimumWidth(250);
    kcmUi.list_quick_widget->rootContext()->setContextObject(new KLocalizedContext(kcmUi.list_quick_widget));
    kcmUi.list_quick_widget->setClearColor(kcmUi.list_quick_widget->palette().color(QPalette::Window));
    kcmUi.list_quick_widget->setSource(QUrl(QStringLiteral("qrc:/kdeconnectkcm/list.qml")));
    kcmUi.list_quick_widget->rootObject()->setProperty("model", QVariant::fromValue(sortProxyModel));
    connect(kcmUi.list_quick_widget->rootObject(), SIGNAL(clicked(QString)), this, SLOT(deviceSelected(QString)));

    kcmUi.list_quick_widget->installEventFilter(new QQuickWidgetPaleteChangeWatcher(kcmUi.list_quick_widget));

    kcmUi.deviceInfo->setVisible(false);
    kcmUi.progressBar->setVisible(false);
    kcmUi.messages->setVisible(false);

    // Workaround: If we set this directly (or if we set it in the .ui file), the layout breaks
    kcmUi.noDeviceLinks->setWordWrap(false);
    QTimer::singleShot(0, this, [this] {
        kcmUi.noDeviceLinks->setWordWrap(true);
    });

    setWhenAvailable(
        daemon->announcedName(),
        [this](bool error, const QString &announcedName) {
            kcmUi.renameShow_button->setEnabled(!error);
            if (error) {
                kcmUi.rename_label->setText(i18n("Error: KDE Connect is not running"));
            } else {
                kcmUi.rename_label->setText(announcedName);
                kcmUi.rename_edit->setText(announcedName);
            }
        },
        this);

    setWhenAvailable(
        daemon->linkProviders(),
        [this](bool error, const QStringList &linkProviders) {
            if (error) {
                return;
            }
            kcmUi.linkProviders_list->clear();
            if (linkProviders.size() <= 1) {
                kcmUi.linkProviders_list->hide();
                kcmUi.linkProviders_label->hide();
            } else {
                for (int i = 0; i < linkProviders.size(); ++i) {
                    const QStringList linkProvider = linkProviders.at(i).split(QStringLiteral("|"));
                    const QString providerId = linkProvider.at(0);
                    QString displayName;
                    if (providerId == QLatin1StringView("BluetoothLinkProvider")) {
                        displayName = i18nc("@info KDE Connect provider name", "Bluetooth");
                    } else if (providerId == QLatin1StringView("LoopbackLinkProvider")) {
                        displayName = i18nc("@info KDE Connect provider name", "Loopback");
                    } else if (providerId == QLatin1StringView("LanLinkProvider")) {
                        displayName = i18nc("@info KDE Connect provider name", "Network");
                    } else {
                        Q_ASSERT_X(false, Q_FUNC_INFO, "Unknow provider given");
                        displayName = i18nc("@info KDE Connect provider name", "Unknown");
                    }
                    QString providerStatus = linkProvider.at(1);

                    QListWidgetItem *linkProviderItem = new QListWidgetItem(displayName, kcmUi.linkProviders_list);
                    linkProviderItem->setData(Qt::UserRole, providerId);

                    if (providerStatus.compare(QStringLiteral("enabled")) == 0) {
                        linkProviderItem->setCheckState(Qt::Checked);
                    } else {
                        linkProviderItem->setCheckState(Qt::Unchecked);
                    }

                    kcmUi.linkProviders_list->addItem(linkProviderItem);
                }
                // Hack to make the list widget resize to fit the contents
                adjustListWidgetHeight(kcmUi.linkProviders_list);
            }

            connect(kcmUi.linkProviders_list, &QListWidget::itemChanged, this, [this](const QListWidgetItem *item) {
                bool checked = item->checkState() == Qt::Checked;
                daemon->setLinkProviderState(item->data(Qt::UserRole).toString(), checked);
            });
        },
        this);

    connect(daemon, &DaemonDbusInterface::announcedNameChanged, kcmUi.rename_edit, &QLineEdit::setText);
    connect(daemon, &DaemonDbusInterface::announcedNameChanged, kcmUi.rename_label, &QLabel::setText);

    connect(daemon, &DaemonDbusInterface::linkProvidersChanged, this, [this](const QStringList &providers) {
        if (kcmUi.linkProviders_list->count() == 0) {
            return; // not yet setup
        }
        for (auto i = 0, count = kcmUi.linkProviders_list->count(); i < count; i++) {
            const auto item = kcmUi.linkProviders_list->item(i);
            const auto id = item->data(Qt::UserRole).toString();

            bool found = false;
            for (const auto &provider : providers) {
                if (provider.startsWith(id)) {
                    const auto status = provider.split(QStringLiteral("|")).at(1);
                    if (status.compare(QStringLiteral("enabled")) == 0) {
                        item->setCheckState(Qt::Checked);
                    } else {
                        item->setCheckState(Qt::Unchecked);
                    }
                    found = true;
                    break;
                }
            }

            Q_ASSERT_X(found, Q_FUNC_INFO, "A new backend appeared, this should not happen as the list of backends is static");
        }
    });

    setRenameMode(false);

    setButtons(KCModule::Help | KCModule::NoAdditionalButton);

    connect(devicesModel, &QAbstractItemModel::rowsRemoved, this, &KdeConnectKcm::devicesRemoved);
    connect(kcmUi.accept_button, &QAbstractButton::clicked, this, &KdeConnectKcm::acceptPairing);
    connect(kcmUi.reject_button, &QAbstractButton::clicked, this, &KdeConnectKcm::cancelPairing);
    connect(kcmUi.cancel_button, &QAbstractButton::clicked, this, &KdeConnectKcm::cancelPairing);
    connect(kcmUi.pair_button, &QAbstractButton::clicked, this, &KdeConnectKcm::requestPairing);
    connect(kcmUi.unpair_button, &QAbstractButton::clicked, this, &KdeConnectKcm::unpair);
    connect(kcmUi.ping_button, &QAbstractButton::clicked, this, &KdeConnectKcm::sendPing);
    connect(kcmUi.refresh_button, &QAbstractButton::clicked, this, &KdeConnectKcm::refresh);
    connect(kcmUi.rename_edit, &QLineEdit::returnPressed, this, &KdeConnectKcm::renameDone);
    connect(kcmUi.renameDone_button, &QAbstractButton::clicked, this, &KdeConnectKcm::renameDone);
    connect(kcmUi.renameShow_button, &QAbstractButton::clicked, this, &KdeConnectKcm::renameShow);
    connect(kcmUi.pluginSelector, &KPluginWidget::changed, this, &KdeConnectKcm::pluginsConfigChanged);

    if (!args.isEmpty() && !args.first().isNull() && args.first().canConvert<QString>()) {
        const QString input = args.first().toString();
        const auto colonIdx = input.indexOf(QLatin1Char(':'));
        const QString deviceId = input.left(colonIdx);
        const QString pluginCM = colonIdx < 0 ? QString() : input.mid(colonIdx + 1);

        connect(devicesModel, &DevicesModel::rowsInserted, this, [this, deviceId, pluginCM]() {
            kcmUi.list_quick_widget->rootObject()->setProperty("currentDeviceId", deviceId);
            deviceSelected(deviceId);
            if (!pluginCM.isEmpty()) {
                kcmUi.pluginSelector->showConfiguration(pluginCM);
            }
            disconnect(devicesModel, &DevicesModel::rowsInserted, this, nullptr);
        });
    }
}

void KdeConnectKcm::devicesRemoved()
{
    if (!currentDevice) {
        return;
    }
    // If current device no longer exists, unselect it
    if (devicesModel->rowForDevice(currentDevice->id()) == -1) {
        delete currentDevice;
        currentDevice = nullptr;
        kcmUi.deviceInfo->setVisible(false);
    }
}

void KdeConnectKcm::renameShow()
{
    setRenameMode(true);
}

void KdeConnectKcm::renameDone()
{
    QString newName = kcmUi.rename_edit->text();
    if (newName.isEmpty()) {
        // Rollback changes
        kcmUi.rename_edit->setText(kcmUi.rename_label->text());
    } else {
        kcmUi.rename_label->setText(newName);
        daemon->setAnnouncedName(newName);
    }
    setRenameMode(false);
}

void KdeConnectKcm::setRenameMode(bool b)
{
    kcmUi.renameDone_button->setVisible(b);
    kcmUi.rename_edit->setVisible(b);
    kcmUi.renameShow_button->setVisible(!b);
    kcmUi.rename_label->setVisible(!b);
}

KdeConnectKcm::~KdeConnectKcm()
{
}

void KdeConnectKcm::refresh()
{
    daemon->forceOnNetworkChange();
}

void KdeConnectKcm::deviceSelected(const QString &deviceId)
{
    if (currentDevice) {
        disconnect(currentDevice, nullptr, this, nullptr);
    }

    currentDevice = devicesModel->getDevice(devicesModel->rowForDevice(deviceId));
    if (!currentDevice) {
        currentDevice = nullptr;
        kcmUi.deviceInfo->setVisible(false);
        return;
    }

    kcmUi.noDevicePlaceholder->setVisible(false);
    bool valid = (currentDevice != nullptr && currentDevice->isValid());
    kcmUi.deviceInfo->setVisible(valid);
    if (!valid) {
        return;
    }

    kcmUi.messages->setVisible(false);
    resetDeviceView();

    connect(currentDevice, &DeviceDbusInterface::pluginsChanged, this, &KdeConnectKcm::resetCurrentDevice);
    connect(currentDevice, &DeviceDbusInterface::pairingFailed, this, &KdeConnectKcm::pairingFailed);
    connect(currentDevice, &DeviceDbusInterface::pairStateChanged, this, &KdeConnectKcm::setCurrentDevicePairState);
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
    kcmUi.name_label->setText(currentDevice->name());
    setWhenAvailable(
        currentDevice->pairStateAsInt(),
        [this](bool error, int pairStateAsInt) {
            if (!error) {
                setCurrentDevicePairState(pairStateAsInt);
            }
        },
        this);

    const QVector<KPluginMetaData> pluginInfo = KPluginMetaData::findPlugins(QStringLiteral("kdeconnect"));
    QVector<KPluginMetaData> availablePluginInfo;

    m_oldSupportedPluginNames = currentDevice->supportedPlugins();
    for (auto it = pluginInfo.cbegin(), itEnd = pluginInfo.cend(); it != itEnd; ++it) {
        if (m_oldSupportedPluginNames.contains(it->pluginId())) {
            availablePluginInfo.append(*it);
        }
    }

    KSharedConfigPtr deviceConfig = KSharedConfig::openConfig(currentDevice->pluginsConfigFile());
    kcmUi.pluginSelector->clear();
    kcmUi.pluginSelector->setConfigurationArguments({currentDevice->id()});
    kcmUi.pluginSelector->addPlugins(availablePluginInfo, i18n("Available plugins"));
    kcmUi.pluginSelector->setConfig(deviceConfig->group(QStringLiteral("Plugins")));
}

void KdeConnectKcm::requestPairing()
{
    if (!currentDevice) {
        return;
    }

    kcmUi.messages->hide();

    currentDevice->requestPairing();
}

void KdeConnectKcm::unpair()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->unpair();
}

void KdeConnectKcm::acceptPairing()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->acceptPairing();
}

void KdeConnectKcm::cancelPairing()
{
    if (!currentDevice) {
        return;
    }

    currentDevice->cancelPairing();
}

void KdeConnectKcm::pairingFailed(const QString &error)
{
    if (sender() != currentDevice)
        return;

    kcmUi.messages->setText(i18n("Error trying to pair: %1", error));
    kcmUi.messages->animatedShow();
}

void KdeConnectKcm::setCurrentDevicePairState(int pairStateAsInt)
{
    PairState state = (PairState)pairStateAsInt; // Hack because qdbus doesn't like enums
    kcmUi.accept_button->setVisible(state == PairState::RequestedByPeer);
    kcmUi.reject_button->setVisible(state == PairState::RequestedByPeer);
    kcmUi.cancel_button->setVisible(state == PairState::Requested);
    kcmUi.pair_button->setVisible(state == PairState::NotPaired);
    kcmUi.unpair_button->setVisible(state == PairState::Paired);
    kcmUi.progressBar->setVisible(state == PairState::Requested);
    kcmUi.ping_button->setVisible(state == PairState::Paired);
    kcmUi.verificationKey->setVisible(state == PairState::Requested || state == PairState::RequestedByPeer);
    switch (state) {
    case PairState::Paired:
        kcmUi.status_label->setText(i18n("(paired)"));
        break;
    case PairState::NotPaired:
        kcmUi.status_label->setText(i18n("(not paired)"));
        break;
    case PairState::RequestedByPeer:
        kcmUi.status_label->setText(i18n("(incoming pair request)"));
        kcmUi.verificationKey->setText(i18n("Key: %1", currentDevice->verificationKey()));
        break;
    case PairState::Requested:
        kcmUi.status_label->setText(i18n("(pairing requested)"));
        kcmUi.verificationKey->setText(i18n("Key: %1", currentDevice->verificationKey()));
        break;
    }
}

void KdeConnectKcm::pluginsConfigChanged(bool changed)
{
    if (!changed)
        return;

    if (!currentDevice)
        return;

    kcmUi.pluginSelector->save();
    currentDevice->reloadPlugins();
}

void KdeConnectKcm::save()
{
    KCModule::save();
}

void KdeConnectKcm::sendPing()
{
    if (!currentDevice)
        return;
    currentDevice->pluginCall(QStringLiteral("ping"), QStringLiteral("sendPing"));
}

#include "kcm.moc"
#include "moc_kcm.cpp"
