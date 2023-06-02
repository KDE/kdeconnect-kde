/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECTKCM_H
#define KDECONNECTKCM_H

#include <KCModule>
#include <QStandardItemModel>

#include <core/pairstate.h>

class QModelIndex;
class DeviceDbusInterface;
class DaemonDbusInterface;
class DevicesModel;
class DevicesSortProxyModel;

namespace Ui
{
class KdeConnectKcmUi;
}

class KdeConnectKcm : public KCModule
{
    Q_OBJECT
public:
    KdeConnectKcm(QWidget *parent, const QVariantList &);
    ~KdeConnectKcm() override;

private:
    void save() override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private Q_SLOTS:
    void deviceSelected(const QModelIndex &current);
    void requestPairing();
    void pluginsConfigChanged();
    void sendPing();
    void resetSelection();
    void pairingFailed(const QString &error);
    void refresh();
    void renameShow();
    void renameDone();
    void setRenameMode(bool b);
    void resetCurrentDevice();
    void setCurrentDevicePairState(PairState pairState);
    void acceptPairing();
    void cancelPairing();


private:
    void resetDeviceView();

    Ui::KdeConnectKcmUi *kcmUi;
    DaemonDbusInterface *daemon;
    DevicesModel *devicesModel;
    DevicesSortProxyModel *sortProxyModel;
    DeviceDbusInterface *currentDevice;
    QModelIndex currentIndex;
    QStringList m_oldSupportedPluginNames;

public Q_SLOTS:
    void unpair();
};

#endif
