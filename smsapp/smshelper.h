/**
 * SPDX-FileCopyrightText: 2019 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SMSHELPER_H
#define SMSHELPER_H

#include <QIcon>
#include <QJSEngine>
#include <QQmlEngine>
#include <QSharedPointer>

#include <KPeople/PersonData>

#include "models/conversationmessage.h"

#include "smsapp/smscharcount.h"

class PersonsCache;

class SmsHelper : public QObject
{
    Q_OBJECT
public:
    static QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);

    enum CountryCode {
        Australia,
        CzechRepublic,
        Other, // I only care about a few country codes
    };

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     */
    Q_INVOKABLE static bool isPhoneNumberMatch(const QString &phone1, const QString &phone2);

    /**
     * Return true to indicate the two phone numbers should be considered the same, false otherwise
     * Requires canonicalized phone numbers as inputs
     */
    Q_INVOKABLE static bool isPhoneNumberMatchCanonicalized(const QString &canonicalPhone1, const QString &canonicalPhone2);

    /**
     * See inline comments for how short codes are determined
     * All information from https://en.wikipedia.org/wiki/Short_code
     */
    Q_INVOKABLE static bool isShortCode(const QString &canonicalNumber, const CountryCode &country);

    /**
     * Try to guess the country code from the passed number
     */
    static CountryCode determineCountryCode(const QString &canonicalNumber);

    /**
     * Simplify a phone number to a known form
     */
    Q_INVOKABLE static QString canonicalizePhoneNumber(const QString &phoneNumber);

    /**
     * Get the data for a particular person given their contact address
     */
    Q_INVOKABLE static QSharedPointer<KPeople::PersonData> lookupPersonByAddress(const QString &address);

    /**
     * Make an icon which combines the many icons
     *
     * This mimics what Android does:
     * If there is only one icon, use that one
     * If there are two icons, put one in the top-left corner and one in the bottom right
     * If there are three, put one in the middle of the top and the remaining two in the bottom
     * If there are four or more, put one in each corner (If more than four, some will be left out)
     */
    Q_INVOKABLE static QIcon combineIcons(const QList<QPixmap> &icons);

    /**
     * Get a combination of all the addresses as a comma-separated list of:
     *  - The KPeople contact's name (if known)
     *  - The address (if the contact is not known)
     */
    Q_INVOKABLE static QString getTitleForAddresses(const QList<ConversationAddress> &addresses);

    /**
     * Get a combined icon for all contacts by finding:
     *  - The KPeople contact's icon (if known)
     *  - A generic icon
     * and then using SmsHelper::combineIcons
     */
    Q_INVOKABLE static QIcon getIconForAddresses(const QList<ConversationAddress> &addresses);

    /**
     * Put the specified text into the system clipboard
     */
    Q_INVOKABLE static void copyToClipboard(const QString &text);

    /**
     * Get the data for all persons currently stored on device
     */
    static QList<QSharedPointer<KPeople::PersonData>> getAllPersons();

    /**
     * Get SMS character count status of SMS. It contains number of remaining characters
     * in current SMS (automatically selects 7-bit, 8-bit or 16-bit mode), octet count and
     * number of messages in concatenated SMS.
     */
    Q_INVOKABLE static SmsCharCount getCharCount(const QString &message);

    /**
     * Used to validate arbitrary phone number entered by the user
     */
    Q_INVOKABLE static bool isAddressValid(const QString &address);

    /**
     * Return the total size of the message
     */
    Q_INVOKABLE static quint64 totalMessageSize(const QList<QUrl> &urls, const QString &text);

    /**
     * Gets a thumbnail for the given attachment
     */
    Q_INVOKABLE static QIcon getThumbnailForAttachment(const Attachment &attachment);

private:
    static bool isInGsmAlphabet(const QChar &ch);
    static bool isInGsmAlphabetExtension(const QChar &ch);
};

#endif // SMSHELPER_H
