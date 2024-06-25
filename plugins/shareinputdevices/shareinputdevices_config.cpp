/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "shareinputdevices_config.h"

#include <interfaces/dbusinterfaces.h>

#include <KPluginFactory>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS(ShareInputDevicesConfig)

constexpr auto defaultEdge = Qt::LeftEdge;
constexpr auto defaultIndex = 1;

ShareInputDevicesConfig::ShareInputDevicesConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
    , m_daemon(new DaemonDbusInterface)
{
    m_ui.setupUi(widget());
    m_ui.edgeContextualWarning->setVisible(false);
    m_ui.edgeComboBox->setItemData(0, Qt::TopEdge);
    m_ui.edgeComboBox->setItemData(1, Qt::LeftEdge);
    m_ui.edgeComboBox->setItemData(2, Qt::RightEdge);
    m_ui.edgeComboBox->setItemData(3, Qt::BottomEdge);
    connect(m_ui.edgeComboBox, &QComboBox::currentIndexChanged, this, &ShareInputDevicesConfig::updateState);
}

void ShareInputDevicesConfig::checkEdge()
{
    m_ui.edgeContextualWarning->setVisible(false);
    const QStringList devices = m_daemon->devices();
    for (const auto &device : devices) {
        if (device == deviceId()) {
            continue;
        }
        if (!DeviceDbusInterface(device).isPluginEnabled(config()->pluginName())) {
            continue;
        }
        if (KdeConnectPluginConfig(device, config()->pluginName()).getInt(u"edge"_s, defaultEdge) == m_ui.edgeComboBox->currentData()) {
            m_ui.edgeContextualWarning->setVisible(true);
        }
    }
}

void ShareInputDevicesConfig::updateState()
{
    const int current = m_ui.edgeComboBox->currentData().toInt();
    unmanagedWidgetChangeState(current != config()->getInt(u"edge"_s, defaultEdge));
    unmanagedWidgetDefaultState(current == defaultEdge);
    checkEdge();
}

void ShareInputDevicesConfig::defaults()
{
    KCModule::defaults();
    m_ui.edgeComboBox->setCurrentIndex(defaultIndex);
    updateState();
}

void ShareInputDevicesConfig::load()
{
    KCModule::load();
    const int index = m_ui.edgeComboBox->findData((config()->getInt(u"edge"_s, defaultEdge)));
    m_ui.edgeComboBox->setCurrentIndex(index != -1 ? index : defaultIndex);
    updateState();
}

void ShareInputDevicesConfig::save()
{
    KCModule::save();
    config()->set(u"edge"_s, m_ui.edgeComboBox->currentData());
    updateState();
}

#include "shareinputdevices_config.moc"
