/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_MDNSH_DISCOVERY_H
#define KDECONNECT_MDNSH_DISCOVERY_H

#include <QObject>

#include "kdeconnectcore_export.h"

#include "mdnsh_wrapper.h"

class LanLinkProvider;

class KDECONNECTCORE_EXPORT MdnshDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit MdnshDiscovery(LanLinkProvider *parent);
    ~MdnshDiscovery();

    void onStart();
    void onStop();

public Q_SLOTS:
    void onNetworkChange();

private:
    MdnshWrapper::Discoverer mdnsDiscoverer;
    MdnshWrapper::Announcer mdnsAnnouncer;
};

#endif // KDECONNECT_SERVER_H
