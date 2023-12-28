/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KCModule>
#include <QStandardItemModel>
#include <kconfigwidgets_version.h>

#include "ui_kcm.h"
#include <core/pairstate.h>

class QModelIndex;
class DeviceDbusInterface;
class DaemonDbusInterface;
class DevicesModel;
class DevicesSortProxyModel;

class KdeConnectKcm : public KCModule
{
    Q_OBJECT
public:
    KdeConnectKcm(QObject *parent, const QVariantList &);
    ~KdeConnectKcm() override;

private:
    void save() override;

private Q_SLOTS:
    void deviceSelected(const QModelIndex &current);
    void requestPairing();
    void pluginsConfigChanged(bool changed);
    void sendPing();
    void resetSelection();
    void pairingFailed(const QString &error);
    void refresh();
    void renameShow();
    void renameDone();
    void setRenameMode(bool b);
    void resetCurrentDevice();
    void setCurrentDevicePairState(int pairStateAsInt);
    void acceptPairing();
    void cancelPairing();

private:
    void resetDeviceView();

    Ui::KdeConnectKcmUi kcmUi;
    DaemonDbusInterface *daemon;
    DevicesModel *devicesModel;
    DevicesSortProxyModel *sortProxyModel;
    DeviceDbusInterface *currentDevice;
    QModelIndex currentIndex;
    QStringList m_oldSupportedPluginNames;

public Q_SLOTS:
    void unpair();
};
