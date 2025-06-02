/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include "networkpacket.h"
#include <QRegularExpression>
#include <QSet>
#include <QSslCertificate>
#include <QString>

struct DeviceType {
    enum Value {
        Unknown,
        Desktop,
        Laptop,
        Phone,
        Tablet,
        Tv,
    };

    static DeviceType FromString(const QString &deviceType)
    {
        if (deviceType == QLatin1String("desktop"))
            return DeviceType::Desktop;
        if (deviceType == QLatin1String("laptop"))
            return DeviceType::Laptop;
        if (deviceType == QLatin1String("smartphone") || deviceType == QLatin1String("phone"))
            return DeviceType::Phone;
        if (deviceType == QLatin1String("tablet"))
            return DeviceType::Tablet;
        if (deviceType == QLatin1String("tv"))
            return DeviceType::Tv;
        return DeviceType::Unknown;
    }

    QString toString() const
    {
        if (value == DeviceType::Desktop)
            return QStringLiteral("desktop");
        if (value == DeviceType::Laptop)
            return QStringLiteral("laptop");
        if (value == DeviceType::Phone)
            return QStringLiteral("phone");
        if (value == DeviceType::Tablet)
            return QStringLiteral("tablet");
        if (value == DeviceType::Tv)
            return QStringLiteral("tv");
        return QStringLiteral("unknown");
    }

    QString icon() const
    {
        return iconForStatus(true, false);
    }

    QString iconForStatus(bool reachable, bool trusted) const
    {
        QString type;
        switch (value) {
        case DeviceType::Unknown:
        case DeviceType::Phone:
            type = QStringLiteral("smartphone");
            break;
        case DeviceType::Desktop: // We don't have desktop icon yet
        case DeviceType::Laptop:
            type = QStringLiteral("laptop");
            break;
        default:
            type = toString();
        }
        QString status = (reachable ? (trusted ? QStringLiteral("connected") : QStringLiteral("disconnected")) : QStringLiteral("trusted"));
        return type + status;
    }

    constexpr DeviceType(Value value)
        : value(value)
    {
    }
    constexpr bool operator==(DeviceType a) const
    {
        return value == a.value;
    }
    constexpr bool operator!=(DeviceType a) const
    {
        return value != a.value;
    }

private:
    Value value;
};

struct DeviceInfo {
    QString id;
    QSslCertificate certificate;
    QString name;
    DeviceType type;
    int protocolVersion;
    QSet<QString> incomingCapabilities;
    QSet<QString> outgoingCapabilities;

    DeviceInfo(const QString &id,
               const QSslCertificate &certificate,
               const QString &name,
               DeviceType type,
               int protocolVersion = 0,
               const QSet<QString> &incomingCapabilities = QSet<QString>(),
               const QSet<QString> &outgoingCapabilities = QSet<QString>())
        : id(id)
        , certificate(certificate)
        , name(name)
        , type(type)
        , protocolVersion(protocolVersion)
        , incomingCapabilities(incomingCapabilities)
        , outgoingCapabilities(outgoingCapabilities)
    {
    }

    NetworkPacket toIdentityPacket()
    {
        NetworkPacket np(PACKET_TYPE_IDENTITY);
        np.set(QStringLiteral("deviceId"), id);
        np.set(QStringLiteral("deviceName"), name);
        np.set(QStringLiteral("deviceType"), type.toString());
        np.set(QStringLiteral("protocolVersion"), protocolVersion);
        np.set(QStringLiteral("incomingCapabilities"), incomingCapabilities.values());
        np.set(QStringLiteral("outgoingCapabilities"), outgoingCapabilities.values());
        return np;
    }

    static DeviceInfo FromIdentityPacketAndCert(const NetworkPacket &np, const QSslCertificate &certificate)
    {
        QStringList incomingCapabilities = np.get<QStringList>(QStringLiteral("incomingCapabilities"));
        QStringList outgoingCapabilities = np.get<QStringList>(QStringLiteral("outgoingCapabilities"));

        return DeviceInfo(np.get<QString>(QStringLiteral("deviceId")),
                          certificate,
                          filterName(np.get<QString>(QStringLiteral("deviceName"))),
                          DeviceType::FromString(np.get<QString>(QStringLiteral("deviceType"))),
                          np.get<int>(QStringLiteral("protocolVersion"), -1),
                          QSet<QString>(incomingCapabilities.begin(), incomingCapabilities.end()),
                          QSet<QString>(outgoingCapabilities.begin(), outgoingCapabilities.end()));
    }

    static QString filterName(QString input)
    {
        static const QRegularExpression NAME_INVALID_CHARACTERS_REGEX(QStringLiteral("[\"',;:.!?()\\[\\]<>]"));
        constexpr int MAX_DEVICE_NAME_LENGTH = 32;
        return input.remove(NAME_INVALID_CHARACTERS_REGEX).left(MAX_DEVICE_NAME_LENGTH);
    }

    static bool isValidIdentityPacket(NetworkPacket *np)
    {
        return np->type() == PACKET_TYPE_IDENTITY && !filterName(np->get(QLatin1String("deviceName"), QString())).isEmpty()
            && isValidDeviceId(np->get(QLatin1String("deviceId"), QString()));
    }

    static bool isValidDeviceId(const QString &deviceId)
    {
        static const QRegularExpression DEVICE_ID_REGEX(QStringLiteral("^[a-zA-Z0-9_-]{32,38}$"));
        return DEVICE_ID_REGEX.match(deviceId).hasMatch();
    }
};

#endif
