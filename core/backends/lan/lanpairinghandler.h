/**
 * Copyright 2015 Vineet Garg <grg.vineet@gmail.com>
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

#ifndef KDECONNECT_LANPAIRINGHANDLER_H
#define KDECONNECT_LANPAIRINGHANDLER_H

#include <QTimer>
#include <backends/devicelink.h>

#include "../../device.h"
#include "../pairinghandler.h"

// This class is used pairing related stuff. It has direct access to links and can directly send packages
class LanPairingHandler
    : public PairingHandler
{

private:
    QTimer m_pairingTimeout;

private Q_SLOTS:
    void pairingTimeout();

public:
    LanPairingHandler(Device* device);
    virtual ~LanPairingHandler() { }

    virtual void createPairPackage(NetworkPackage& np) Q_DECL_OVERRIDE;
    virtual bool packageReceived(const NetworkPackage& np) Q_DECL_OVERRIDE;
    virtual bool requestPairing() Q_DECL_OVERRIDE;
    virtual bool acceptPairing() Q_DECL_OVERRIDE;
    virtual void rejectPairing() Q_DECL_OVERRIDE;
    virtual void pairingDone() Q_DECL_OVERRIDE;
    virtual void unpair() Q_DECL_OVERRIDE;


};


#endif //KDECONNECT_LANPAIRINGHANDLER_H
