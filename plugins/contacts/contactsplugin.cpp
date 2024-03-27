/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <contactsplugin.h>

#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDir>
#include <QFile>
#include <QIODevice>

#include <core/device.h>

#include "plugin_contacts_debug.h"

K_PLUGIN_CLASS_WITH_JSON(ContactsPlugin, "kdeconnect_contacts.json")

ContactsPlugin::ContactsPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , vcardsPath(QString(*vcardsLocation).append(QStringLiteral("/kdeconnect-").append(device()->id())))
{
    // Register custom types with dbus
    qRegisterMetaType<uID>("uID");
    qDBusRegisterMetaType<uID>();

    qRegisterMetaType<uIDList_t>("uIDList_t");
    qDBusRegisterMetaType<uIDList_t>();

    // Create the storage directory if it doesn't exist
    if (!QDir().mkpath(vcardsPath)) {
        qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "Unable to create VCard directory";
    }

    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Contacts constructor for device " << device()->name();
}

void ContactsPlugin::connected()
{
    synchronizeRemoteWithLocal();
}

void ContactsPlugin::receivePacket(const NetworkPacket &np)
{
    // qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Packet Received for device " << device()->name();
    // qCDebug(KDECONNECT_PLUGIN_CONTACTS) << np.body();

    if (np.type() == PACKAGE_TYPE_CONTACTS_RESPONSE_UIDS_TIMESTAMPS) {
        handleResponseUIDsTimestamps(np);
    } else if (np.type() == PACKET_TYPE_CONTACTS_RESPONSE_VCARDS) {
        handleResponseVCards(np);
    }
}

void ContactsPlugin::synchronizeRemoteWithLocal()
{
    sendRequest(PACKET_TYPE_CONTACTS_REQUEST_ALL_UIDS_TIMESTAMP);
}

bool ContactsPlugin::handleResponseUIDsTimestamps(const NetworkPacket &np)
{
    if (!np.has(QStringLiteral("uids"))) {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDsTimestamps:" << "Malformed packet does not have uids key";
        return false;
    }
    uIDList_t uIDsToUpdate;
    QDir vcardsDir(vcardsPath);

    // Get a list of all file info in this directory
    // Clean out IDs returned from the remote. Anything leftover should be deleted
    QFileInfoList localVCards = vcardsDir.entryInfoList({QStringLiteral("*.vcard"), QStringLiteral("*.vcf")});

    const QStringList &uIDs = np.get<QStringList>(QStringLiteral("uids"));

    // Check local storage for the contacts:
    //  If the contact is not found in local storage, request its vcard be sent
    //  If the contact is in local storage but not reported, delete it
    //  If the contact is in local storage, compare its timestamp. If different, request the contact
    for (const QString &ID : uIDs) {
        QString filename = vcardsDir.filePath(ID + VCARD_EXTENSION);
        QFile vcardFile(filename);

        if (!QFile().exists(filename)) {
            // We do not have a vcard for this contact. Request it.
            uIDsToUpdate.push_back(ID);
            continue;
        }

        // Remove this file from the list of known files
        QFileInfo fileInfo(vcardFile);
        localVCards.removeOne(fileInfo);

        // Check if the vcard needs to be updated
        if (!vcardFile.open(QIODevice::ReadOnly)) {
            qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseUIDsTimestamps:" << "Unable to open" << filename
                                                  << "to read even though it was reported to exist";
            continue;
        }

        QTextStream fileReadStream(&vcardFile);
        QString line;
        while (!fileReadStream.atEnd()) {
            fileReadStream >> line;
            // TODO: Check that the saved ID is the same as the one we were expecting. This requires parsing the VCard
            if (!line.startsWith(QStringLiteral("X-KDECONNECT-TIMESTAMP:"))) {
                continue;
            }
            QStringList parts = line.split(QLatin1Char(':'));
            QString timestamp = parts[1];

            qint64 remoteTimestamp = np.get<qint64>(ID);
            qint64 localTimestamp = timestamp.toLongLong();

            if (!(localTimestamp == remoteTimestamp)) {
                uIDsToUpdate.push_back(ID);
            }
        }
    }

    // Delete all locally-known files which were not reported by the remote device
    for (const QFileInfo &unknownFile : localVCards) {
        QFile toDelete(unknownFile.filePath());
        toDelete.remove();
    }

    sendRequestWithIDs(PACKET_TYPE_CONTACTS_REQUEST_VCARDS_BY_UIDS, uIDsToUpdate);

    return true;
}

bool ContactsPlugin::handleResponseVCards(const NetworkPacket &np)
{
    if (!np.has(QStringLiteral("uids"))) {
        qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Malformed packet does not have uids key";
        return false;
    }

    QDir vcardsDir(vcardsPath);
    const QStringList &uIDs = np.get<QStringList>(QStringLiteral("uids"));

    // Loop over all IDs, extract the VCard from the packet and write the file
    for (const auto &ID : uIDs) {
        // qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "Got VCard:" << np.get<QString>(ID);
        QString filename = vcardsDir.filePath(ID + VCARD_EXTENSION);
        QFile vcardFile(filename);
        bool vcardFileOpened = vcardFile.open(QIODevice::WriteOnly); // Want to smash anything that might have already been there
        if (!vcardFileOpened) {
            qCWarning(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Unable to open" << filename;
            continue;
        }

        QTextStream fileWriteStream(&vcardFile);
        const QString &vcard = np.get<QString>(ID);
        fileWriteStream << vcard;
    }
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "handleResponseVCards:" << "Got" << uIDs.size() << "VCards";
    Q_EMIT localCacheSynchronized(uIDs);
    return true;
}

bool ContactsPlugin::sendRequest(const QString &packetType)
{
    NetworkPacket np(packetType);
    bool success = sendPacket(np);
    qCDebug(KDECONNECT_PLUGIN_CONTACTS) << "sendRequest: Sending " << packetType << success;

    return success;
}

bool ContactsPlugin::sendRequestWithIDs(const QString &packetType, const uIDList_t &uIDs)
{
    NetworkPacket np(packetType);

    np.set<uIDList_t>(QStringLiteral("uids"), uIDs);
    bool success = sendPacket(np);
    return success;
}

QString ContactsPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/contacts").arg(device()->id());
}

#include "contactsplugin.moc"
#include "moc_contactsplugin.cpp"
