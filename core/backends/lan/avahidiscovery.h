/**
 * SPDX-FileCopyrightText: 2025 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "kdeconnectcore_export.h"

#include "generated/systeminterfaces/avahientrygroup.h"
#include "generated/systeminterfaces/avahiserver.h"
#include "generated/systeminterfaces/avahiservicerbrowser.h"

class LanLinkProvider;

class KDECONNECTCORE_EXPORT AvahiDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit AvahiDiscovery(LanLinkProvider *lanLinkProvider);
    ~AvahiDiscovery();

    void onStart();
    void onStop();

public Q_SLOTS:
    void onNetworkChange();

private:
    void startAnnouncing();
    void stopAnnouncing();
    void startDiscovering();
    void stopDiscovering();

    OrgFreedesktopAvahiServer2Interface m_avahiServerInterface;
    OrgFreedesktopAvahiServiceBrowserInterface *m_serviceBrowserInterface;
    OrgFreedesktopAvahiEntryGroupInterface *m_entryGroupInterface;
    LanLinkProvider *lanLinkProvider;
};
