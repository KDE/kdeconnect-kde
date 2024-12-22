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
    LoopbackLinkProvider();
    ~LoopbackLinkProvider() override;

    QString name() override
    {
        return QStringLiteral("LoopbackLinkProvider");
    }
    int priority() override
    {
        return 0;
    }

    void enable() override
    {
    }

    void disable() override
    {
    }

    void onStart() override;
    void onStop() override;
    void onNetworkChange() override;
    void onLinkDestroyed(const QString &, DeviceLink *) override
    {
    }

private:
    QPointer<LoopbackDeviceLink> loopbackDeviceLink;
};

#endif
