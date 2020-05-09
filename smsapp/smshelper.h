/**
 * Copyright (C) 2019 Simon Redman <simon@ergotech.com>
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

#ifndef SMSHELPER_H
#define SMSHELPER_H

#include <QIcon>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QSharedPointer>

#include <KPeople/PersonData>

#include "interfaces/conversationmessage.h"

#include "kdeconnectsms_export.h"
#include "smsapp/smscharcount.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_SMSHELPER)

class PersonsCache;

class KDECONNECTSMSAPPLIB_EXPORT SmsHelper : public QObject
{
    Q_OBJECT
public:
    SmsHelper() = default;
    ~SmsHelper() = default;

    static QObject* singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);

    enum CountryCode {
        Australia,
        CzechRepublic,
        Other, // I only care about a few country codes
    };

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     */
    Q_INVOKABLE static bool isPhoneNumberMatch(const QString& phone1, const QString& phone2);

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     * Requires canonicalized phone numbers as inputs
     */
    Q_INVOKABLE static bool isPhoneNumberMatchCanonicalized(const QString& canonicalPhone1, const QString& canonicalPhone2);

    /**
     * See inline comments for how short codes are determined
     * All information from https://en.wikipedia.org/wiki/Short_code
     */
    Q_INVOKABLE static bool isShortCode(const QString& canonicalNumber, const CountryCode& country);

    /**
     * Try to guess the country code from the passed number
     */
    static CountryCode determineCountryCode(const QString& canonicalNumber);

    /**
     * Simplify a phone number to a known form
     */
    Q_INVOKABLE static QString canonicalizePhoneNumber(const QString& phoneNumber);

    /**
     * Get the data for a particular person given their contact address
     */
    Q_INVOKABLE static QSharedPointer<KPeople::PersonData> lookupPersonByAddress(const QString& address);

    /**
     * Make an icon which combines the many icons
     *
     * This mimics what Android does:
     * If there is only one icon, use that one
     * If there are two icons, put one in the top-left corner and one in the bottom right
     * If there are three, put one in the middle of the top and the remaining two in the bottom
     * If there are four or more, put one in each corner (If more than four, some will be left out)
     */
    Q_INVOKABLE static QIcon combineIcons(const QList<QPixmap>& icons);

    /**
     * Get a combination of all the addresses as a comma-separated list of:
     *  - The KPeople contact's name (if known)
     *  - The address (if the contact is not known)
     */
    Q_INVOKABLE static QString getTitleForAddresses(const QList<ConversationAddress>& addresses);

    /**
     * Get a combined icon for all contacts by finding:
     *  - The KPeople contact's icon (if known)
     *  - A generic icon
     * and then using SmsHelper::combineIcons
     */
    Q_INVOKABLE static QIcon getIconForAddresses(const QList<ConversationAddress>& addresses);

    /**
    * Put the specified text into the system clipboard
    */
    Q_INVOKABLE static void copyToClipboard(const QString& text);

    /**
     * Get the data for all persons currently stored on device
     */
    static QList<QSharedPointer<KPeople::PersonData>> getAllPersons();

    /**
     * Get SMS character count status of SMS. It contains number of remaining characters
     * in current SMS (automatically selects 7-bit, 8-bit or 16-bit mode), octet count and
     * number of messages in concatenated SMS.
     */
    Q_INVOKABLE static SmsCharCount getCharCount(const QString& message);

    /**
     * Used to validate arbitrary phone number entered by the user
     */
    Q_INVOKABLE static bool isAddressValid(const QString& address);

private:
    static bool isInGsmAlphabet(const QChar& ch);
    static bool isInGsmAlphabetExtension(const QChar& ch);
};

#endif // SMSHELPER_H
