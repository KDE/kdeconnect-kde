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

#include "landevicelink.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "../linkprovider.h"
#include "uploadjob.h"
#include "downloadjob.h"
#include "socketlinereader.h"

LanDeviceLink::LanDeviceLink(const QString& d, LinkProvider* a, QTcpSocket* socket)
    : DeviceLink(d, a)
    , mSocketLineReader(new SocketLineReader(socket, a))
{

    connect(mSocketLineReader, SIGNAL(disconnected()),
            this, SLOT(deleteLater()));
    connect(mSocketLineReader, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));
}

bool LanDeviceLink::sendPackageEncrypted(QCA::PublicKey& key, NetworkPackage& np)
{
    if (np.hasPayload()) {
         UploadJob* job = new UploadJob(np.payload());
         job->start();
         np.setPayloadTransferInfo(job->getTransferInfo());
    }

    np.encrypt(key);

    int written = mSocketLineReader->write(np.serialize());

    //TODO: Actually detect if a package is received or not, now we keep TCP
    //"ESTABLISHED" connections that look legit (return true when we use them),
    //but that are actually broken
    return (written != -1);
}

bool LanDeviceLink::sendPackage(NetworkPackage& np)
{
    if (np.hasPayload()) {
         UploadJob* job = new UploadJob(np.payload());
         job->start();
         np.setPayloadTransferInfo(job->getTransferInfo());
    }

    int written = mSocketLineReader->write(np.serialize());
    //TODO: Actually detect if a package is received or not, now we keep TCP
    //"ESTABLISHED" connections that look legit (return true when we use them),
    //but that are actually broken
    return (written != -1);
}

void LanDeviceLink::dataReceived()
{

    if (mSocketLineReader->bytesAvailable() == 0) return;

    QByteArray package = mSocketLineReader->readLine();

    //kDebug(kdeconnect_kded()) << "LanDeviceLink dataReceived" << package;

    NetworkPackage unserialized(QString::null);
    NetworkPackage::unserialize(package, &unserialized);
    if (unserialized.isEncrypted()) {

        //mPrivateKey should always be set when device link is added to device, no null-checking done here
        NetworkPackage decrypted(QString::null);
        unserialized.decrypt(mPrivateKey, &decrypted);

        if (decrypted.hasPayloadTransferInfo()) {
            kDebug(kdeconnect_kded()) << "HasPayloadTransferInfo";
            DownloadJob* job = new DownloadJob(mSocketLineReader->peerAddress(), decrypted.payloadTransferInfo());
            job->start();
            decrypted.setPayload(job->getPayload(), decrypted.payloadSize());
        }

        Q_EMIT receivedPackage(decrypted);

    } else {

        if (unserialized.hasPayloadTransferInfo()) {
            qWarning() << "Ignoring unencrypted payload";
        }

        Q_EMIT receivedPackage(unserialized);

    }

    if (mSocketLineReader->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }

}
