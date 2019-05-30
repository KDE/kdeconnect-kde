/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONVERSATIONLISTMODEL_H
#define CONVERSATIONLISTMODEL_H

#include <QStandardItemModel>
#include <QLoggingCategory>
#include <QQmlParserStatus>
#include <KPeople/kpeople/personsmodel.h>
#include <KPeople/kpeople/persondata.h>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

#include "kdeconnectsms_export.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL)


class KDECONNECTSMSAPPLIB_EXPORT OurSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder)
public:

    Qt::SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(Qt::SortOrder sortOrder) {
        if (m_sortOrder != sortOrder) {
            m_sortOrder = sortOrder;
            sortNow();
        }
    }
    void classBegin() override {}
    void componentComplete() override {
        m_completed = true;
        sortNow();
    }

    OurSortFilterProxyModel();
    ~OurSortFilterProxyModel();

private:
    void sortNow() {
        if (m_completed && dynamicSortFilter())
            sort(0, m_sortOrder);
    }

    bool m_completed = false;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

class KDECONNECTSMSAPPLIB_EXPORT ConversationListModel
    : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    ConversationListModel(QObject* parent = nullptr);
    ~ConversationListModel();

    enum Roles {
        FromMeRole = Qt::UserRole,
        PersonUriRole,
        AddressRole,
        ConversationIdRole,
        DateRole,
    };
    Q_ENUM(Roles)

    enum CountryCode {
        Australia,
        CzechRepublic,
        Other, // I only care about a few country codes
    };

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &/*deviceId*/);

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     */
    static bool isPhoneNumberMatch(const QString& phone1, const QString& phone2);

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     * Requires canonicalized phone numbers as inputs
     */
    static bool isPhoneNumberMatchCanonicalized(const QString& canonicalPhone1, const QString& canonicalPhone2);

    /**
     * See inline comments for how short codes are determined
     * All information from https://en.wikipedia.org/wiki/Short_code
     */
    static bool isShortCode(const QString& canonicalNumber, const CountryCode& country);

    /**
     * Try to guess the country code from the passed number
     */
    static CountryCode determineCountryCode(const QString& canonicalNumber);

    /**
     * Simplify a phone number to a known form
     */
    static QString canonicalizePhoneNumber(const QString& phoneNumber);

public Q_SLOTS:
    void handleCreatedConversation(const QVariantMap& msg);
    void handleConversationUpdated(const QVariantMap& msg);
    void createRowFromMessage(const QVariantMap& message);
    void printDBusError(const QDBusError& error);

Q_SIGNALS:
    void deviceIdChanged();

private:
    /**
     * Get all conversations currently known by the conversationsInterface, if any
     */
    void prepareConversationsList();

    /**
     * Get the data for a particular person given their contact address
     */
    KPeople::PersonData* lookupPersonByAddress(const QString& address);

    QStandardItem* conversationForThreadId(qint32 threadId);

    DeviceConversationsDbusInterface* m_conversationsInterface;
    QString m_deviceId;
    KPeople::PersonsModel m_people;
};

#endif // CONVERSATIONLISTMODEL_H
