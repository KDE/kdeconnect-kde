/**
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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

#include <contactsplugin.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QtDBus>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QFile>
#include <QDir>
#include <QIODevice>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON(KdeConnectPluginFactory, "kdeconnect_contacts.json",
                           registerPlugin<ContactsPlugin>(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_CONTACTS, "kdeconnect.plugin.contacts")

ContactsPlugin::ContactsPlugin (QObject* parent, const QVariantList& args) :
        KdeConnectPlugin(parent, args)
{
    vcardsPath = QString(*vcardsLocation).append("/kdeconnect-").append(device()->id());

    // Register custom types with dbus
    qRegisterMetaType<uID>("uID");
    qDBusRegisterMetaType<uID>();

    qRegisterMetaType<uIDList_t>("uIDList_t");
    qDBusRegisterMetaType<uIDList_t>();

    // Create the storage directory if it doesn't exist
    if (!QDir().mkpath(vcardsPath)) {
        qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Unable to create VCard directory";
    }

    this->synchronizeRemoteWithLocal();

    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts constructor for device " << device()->name();
}

ContactsPlugin::~ContactsPlugin () {
    QDBusConnection::sessionBus().unregisterObject(dbusPath(), QDBusConnection::UnregisterTree);
//     qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts plugin destructor for device" << device()->name();
}

bool ContactsPlugin::receivePacket (const NetworkPacket& np) {
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Packet Received for device " << device()->name();
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << np.body();

    if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS_TIMESTAMPS) {
        return this->handleResponseUIDsTimestamps(np);
    } else if (np.type() == PACKET_TYPE_CONTACTS_RESPONSE_VCARDS) {
        return this->handleResponseVCards(np);
    } else {
        // Is this check necessary?
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Unknown package type received from device: "
                << device()->name() << ". Maybe you need to upgrade KDE Connect?";
        return false;
    }
}

void ContactsPlugin::synchronizeRemoteWithLocal () {
    this->sendRequest(PACKET_TYPE_CONTACTS_REQUEST_ALL_UIDS_TIMESTAMP);
}

bool ContactsPlugin::handleResponseUIDsTimestamps (const NetworkPacket& np) {
    if (!np.has("uids")) {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDsTimestamps:"
                << "Malformed packet does not have uids key";
        return false;
    }
    uIDList_t uIDsToUpdate;
    QDir vcardsDir(vcardsPath);

    // Get a list of all file info in this directory
    // Clean out IDs returned from the remote. Anything leftover should be deleted
    QFileInfoList localVCards = vcardsDir.entryInfoList( { "*.vcard", "*.vcf" });

    const QStringList& uIDs = np.get<QStringList>("uids");

    // Check local storage for the contacts:
    //  If the contact is not found in local storage, request its vcard be sent
    //  If the contact is in local storage but not reported, delete it
    //  If the contact is in local storage, compare its timestamp. If different, request the contact
    for (const QString& ID : uIDs) {
        QString filename = vcardsDir.filePath(ID + VCARD_EXTENSION);
        QFile vcardFile(filename);

        if (!QFile().exists(filename)) {
            // We do not have a vcard for this contact. Request it.
            uIDsToUpdate.push_back(ID);
            continue;
        }

        // Remove this file from the list of known files
        QFileInfo fileInfo(vcardFile);
        bool success = localVCards.removeOne(fileInfo);
        Q_ASSERT(success); // We should have always been able to remove the existing file from our listing

        // Check if the vcard needs to be updated
        if (!vcardFile.open(QIODevice::ReadOnly)) {
            qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDsTimestamps:"
                    << "Unable to open" << filename << "to read even though it was reported to exist";
            continue;
        }

        QTextStream fileReadStream(&vcardFile);
        QString line;
        while (!fileReadStream.atEnd()) {
            fileReadStream >> line;
            // TODO: Check that the saved ID is the same as the one we were expecting. This requires parsing the VCard
            if (!line.startsWith("X-KDECONNECT-TIMESTAMP:")) {
                continue;
            }
            QStringList parts = line.split(":");
            QString timestamp = parts[1];

            qint32 remoteTimestamp = np.get<qint32>(ID);
            qint32 localTimestamp = timestamp.toInt();

            if (!(localTimestamp == remoteTimestamp)) {
                uIDsToUpdate.push_back(ID);
            }
        }
    }

    // Delete all locally-known files which were not reported by the remote device
    for (const QFileInfo& unknownFile : localVCards) {
        QFile toDelete(unknownFile.filePath());
        toDelete.remove();
    }

    this->sendRequestWithIDs(PACKET_TYPE_CONTACTS_REQUEST_VCARDS_BY_UIDS, uIDsToUpdate);

    return true;
}

bool ContactsPlugin::handleResponseVCards (const NetworkPacket& np) {
    if (!np.has("uids")) {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS)
        << "handleResponseVCards:" << "Malformed packet does not have uids key";
        return false;
    }

    QDir vcardsDir(vcardsPath);
    const QStringList& uIDs = np.get<QStringList>("uids");

    // Loop over all IDs, extract the VCard from the packet and write the file
    for (const auto& ID : uIDs) {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Got VCard:" << np.get<QString>(ID);
        QString filename = vcardsDir.filePath(ID + VCARD_EXTENSION);
        QFile vcardFile(filename);
        bool vcardFileOpened = vcardFile.open(QIODevice::WriteOnly); // Want to smash anything that might have already been there
        if (!vcardFileOpened) {
            qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Unable to open" << filename;
            continue;
        }

        QTextStream fileWriteStream(&vcardFile);
        const QString& vcard = np.get<QString>(ID);
        fileWriteStream << vcard;
    }
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Got" << uIDs.size() << "VCards";
    Q_EMIT localCacheSynchronized(uIDs);
    return true;
}

bool ContactsPlugin::sendRequest (const QString& packetType) {
    NetworkPacket np(packetType);
    bool success = sendPacket(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendRequest: Sending " << packetType << success;

    return success;
}

bool ContactsPlugin::sendRequestWithIDs (const QString& packetType, const uIDList_t& uIDs) {
    NetworkPacket np(packetType);

    np.set<uIDList_t>("uids", uIDs);
    bool success = sendPacket(np);
    return success;
}

QString ContactsPlugin::dbusPath () const {
    return "/modules/kdeconnect/devices/" + device()->id() + "/contacts";
}

#include "contactsplugin.moc"

