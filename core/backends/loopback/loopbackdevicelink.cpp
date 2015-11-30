/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "loopbackdevicelink.h"

#include "loopbacklinkprovider.h"
#include "loopbackpairinghandler.h"

LoopbackDeviceLink::LoopbackDeviceLink(const QString& deviceId, LoopbackLinkProvider* provider)
    : DeviceLink(deviceId, provider, Remotely)
{

}

QString LoopbackDeviceLink::name()
{
    return "LoopbackLink"; // Should be similar to android
}

PairingHandler* LoopbackDeviceLink::createPairingHandler(Device *device)
{
    return new LoopbackPairingHandler(device);
}
bool LoopbackDeviceLink::sendPackageEncrypted(QCA::PublicKey& key, NetworkPackage& input)
{
    if (mPrivateKey.isNull() || key.isNull()) {
        return false;
    }

    input.encrypt(key);

    QByteArray serialized = input.serialize();

    NetworkPackage unserialized(QString::null);
    NetworkPackage::unserialize(serialized, &unserialized);

    NetworkPackage output(QString::null);
    unserialized.decrypt(mPrivateKey, &output);

    bool b = true;
    //LoopbackDeviceLink does not need deviceTransferInfo
    if (input.hasPayload()) {
        b = input.payload()->open(QIODevice::ReadOnly);
        Q_ASSERT(b);
        output.setPayload(input.payload(), input.payloadSize());
    }

    Q_EMIT receivedPackage(output);

    return b;
}

bool LoopbackDeviceLink::sendPackage(NetworkPackage& input)
{
    NetworkPackage output(QString::null);
    NetworkPackage::unserialize(input.serialize(), &output);

    //LoopbackDeviceLink does not need deviceTransferInfo
    if (input.hasPayload()) {
        bool b = input.payload()->open(QIODevice::ReadOnly);
        Q_ASSERT(b);
        output.setPayload(input.payload(), input.payloadSize());
    }

    Q_EMIT receivedPackage(output);

    return true;
}

