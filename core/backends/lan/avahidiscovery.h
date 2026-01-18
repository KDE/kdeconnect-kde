/**
 * SPDX-FileCopyrightText: 2025 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <memory>

#include "kdeconnectcore_export.h"
#include "mdnsdiscovery.h"

#include "generated/systeminterfaces/avahientrygroup.h"
#include "generated/systeminterfaces/avahiserver.h"
#include "generated/systeminterfaces/avahiservicerbrowser.h"

class LanLinkProvider;

struct AvahiServiceBrowserDeleter {
    void operator()(OrgFreedesktopAvahiServiceBrowserInterface *ptr) const;
};

struct AvahiEntryGroupDeleter {
    void operator()(OrgFreedesktopAvahiEntryGroupInterface *ptr) const;
};

class KDECONNECTCORE_EXPORT AvahiDiscovery : public QObject, public MdnsDiscovery
{
    Q_OBJECT

public:
    explicit AvahiDiscovery(LanLinkProvider *lanLinkProvider);
    ~AvahiDiscovery() override;

    static bool hasAvahiDaemonRunning();

    void onStart() override;
    void onStop() override;
    void onNetworkChange() override;

private:
    void startAnnouncing();
    void stopAnnouncing();
    void startDiscovering();
    void stopDiscovering();

    OrgFreedesktopAvahiServer2Interface m_avahiServerInterface;
    std::unique_ptr<OrgFreedesktopAvahiServiceBrowserInterface, AvahiServiceBrowserDeleter> m_serviceBrowserInterface;
    std::unique_ptr<OrgFreedesktopAvahiEntryGroupInterface, AvahiEntryGroupDeleter> m_entryGroupInterface;
    LanLinkProvider *lanLinkProvider;
};
