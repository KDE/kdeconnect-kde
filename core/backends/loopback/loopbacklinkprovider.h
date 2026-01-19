/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LOOPBACKLINKPROVIDER_H
#define LOOPBACKLINKPROVIDER_H

#include "../linkprovider.h"
#include "loopbackdevicelink.h"
#include <QPointer>

class LoopbackLinkProvider : public LinkProvider
{
    Q_OBJECT
public:
    LoopbackLinkProvider(bool disabled);
    ~LoopbackLinkProvider() override;

    QString name() override
    {
        return QStringLiteral("LoopbackLinkProvider");
    }

    QString displayName() override
    {
        return i18nc("@info", "Loopback");
    }

    int priority() override
    {
        return 0;
    }

    void enable() override
    {
        enabled = true;
        onNetworkChange();
    }

    void disable() override
    {
        enabled = false;
        onNetworkChange();
    }

    void onStart() override;
    void onStop() override;
    void onNetworkChange() override;
    void deviceRemoved(const QString & /*device*/) override
    {
    }

private:
    QPointer<LoopbackDeviceLink> loopbackDeviceLink;
    bool enabled;
};

#endif
