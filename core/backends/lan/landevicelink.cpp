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

#include <KLocalizedString>

#include "landevicelink.h"
#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "backends/linkprovider.h"
#include "uploadjob.h"
#include "downloadjob.h"
#include "socketlinereader.h"
#include "lanlinkprovider.h"

LanDeviceLink::LanDeviceLink(const QString& deviceId, LinkProvider* parent, QSslSocket* socket, ConnectionStarted connectionSource)
    : DeviceLink(deviceId, parent)
    , mSocketLineReader(nullptr)
{
    reset(socket, connectionSource);
}

void LanDeviceLink::reset(QSslSocket* socket, ConnectionStarted connectionSource)
{
    if (mSocketLineReader) {
        disconnect(mSocketLineReader->mSocket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
        delete mSocketLineReader;
    }

    mSocketLineReader = new SocketLineReader(socket, this);

    connect(socket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
    connect(mSocketLineReader, &SocketLineReader::readyRead, this, &LanDeviceLink::dataReceived);

    //We take ownership of the socket.
    //When the link provider destroys us,
    //the socket (and the reader) will be
    //destroyed as well
    socket->setParent(mSocketLineReader);

    mConnectionSource = connectionSource;

    QString certString = KdeConnectConfig::instance()->getDeviceProperty(deviceId(), QStringLiteral("certificate"));
    DeviceLink::setPairStatus(certString.isEmpty()? PairStatus::NotPaired : PairStatus::Paired);
}

QString LanDeviceLink::name()
{
    return QStringLiteral("LanLink"); // Should be same in both android and kde version
}

bool LanDeviceLink::sendPackage(NetworkPackage& np)
{
    if (np.hasPayload()) {
        np.setPayloadTransferInfo(sendPayload(np)->transferInfo());
    }

    int written = mSocketLineReader->write(np.serialize());

    //Actually we can't detect if a package is received or not. We keep TCP
    //"ESTABLISHED" connections that look legit (return true when we use them),
    //but that are actually broken (until keepalive detects that they are down).
    return (written != -1);
}

UploadJob* LanDeviceLink::sendPayload(const NetworkPackage& np)
{
    UploadJob* job = new UploadJob(np.payload(), deviceId());
    job->start();
    return job;
}

void LanDeviceLink::dataReceived()
{
    if (mSocketLineReader->bytesAvailable() == 0) return;

    const QByteArray serializedPackage = mSocketLineReader->readLine();
    NetworkPackage package(QString::null);
    NetworkPackage::unserialize(serializedPackage, &package);

    //qCDebug(KDECONNECT_CORE) << "LanDeviceLink dataReceived" << serializedPackage;

    if (package.type() == PACKAGE_TYPE_PAIR) {
        //TODO: Handle pair/unpair requests and forward them (to the pairing handler?)
        qobject_cast<LanLinkProvider*>(provider())->incomingPairPackage(this, package);
        return;
    }

    if (package.hasPayloadTransferInfo()) {
        //qCDebug(KDECONNECT_CORE) << "HasPayloadTransferInfo";
        QVariantMap transferInfo = package.payloadTransferInfo();
        //FIXME: The next two lines shouldn't be needed! Why are they here?
        transferInfo.insert(QStringLiteral("useSsl"), true);
        transferInfo.insert(QStringLiteral("deviceId"), deviceId());
        DownloadJob* job = new DownloadJob(mSocketLineReader->peerAddress(), transferInfo);
        job->start();
        package.setPayload(job->getPayload(), package.payloadSize());
    }

    Q_EMIT receivedPackage(package);

    if (mSocketLineReader->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }

}

void LanDeviceLink::userRequestsPair()
{
    if (mSocketLineReader->peerCertificate().isNull()) {
        Q_EMIT pairingError(i18n("This device cannot be paired because it is running an old version of KDE Connect."));
    } else {
        qobject_cast<LanLinkProvider*>(provider())->userRequestsPair(deviceId());
    }
}

void LanDeviceLink::userRequestsUnpair()
{
    qobject_cast<LanLinkProvider*>(provider())->userRequestsUnpair(deviceId());
}

void LanDeviceLink::setPairStatus(PairStatus status)
{
    if (status == Paired && mSocketLineReader->peerCertificate().isNull()) {
        Q_EMIT pairingError(i18n("This device cannot be paired because it is running an old version of KDE Connect."));
        return;
    }

    DeviceLink::setPairStatus(status);
    if (status == Paired) {
        Q_ASSERT(KdeConnectConfig::instance()->trustedDevices().contains(deviceId()));
        Q_ASSERT(!mSocketLineReader->peerCertificate().isNull());
        KdeConnectConfig::instance()->setDeviceProperty(deviceId(), QStringLiteral("certificate"), mSocketLineReader->peerCertificate().toPem());
    }
}

bool LanDeviceLink::linkShouldBeKeptAlive() {

    return true;     //FIXME: Current implementation is broken, so for now we will keep links always established

    //We keep the remotely initiated connections, since the remotes require them if they want to request
    //pairing to us, or connections that are already paired. TODO: Keep connections in the process of pairing
    //return (mConnectionSource == ConnectionStarted::Remotely || pairStatus() == Paired);

}
