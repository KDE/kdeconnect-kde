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

#include "smshelper.h"

#include <QIcon>
#include <QPainter>
#include <QRegularExpression>
#include <QString>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QHash>

#include <KPeople/PersonData>
#include <KPeople/PersonsModel>

#include "interfaces/conversationmessage.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_SMSHELPER, "kdeconnect.sms.smshelper")

bool SmsHelper::isPhoneNumberMatchCanonicalized(const QString& canonicalPhone1, const QString& canonicalPhone2)
{
    if (canonicalPhone1.isEmpty() || canonicalPhone2.isEmpty()) {
        // The empty string is not a valid phone number so does not match anything
        return false;
    }

    // To decide if a phone number matches:
    // 1. Are they similar lengths? If two numbers are very different, probably one is junk data and should be ignored
    // 2. Is one a superset of the other? Phone number digits get more specific the further towards the end of the string,
    //    so if one phone number ends with the other, it is probably just a more-complete version of the same thing
    const QString& longerNumber = canonicalPhone1.length() >= canonicalPhone2.length() ? canonicalPhone1 : canonicalPhone2;
    const QString& shorterNumber = canonicalPhone1.length() < canonicalPhone2.length() ? canonicalPhone1 : canonicalPhone2;

    const CountryCode& country = determineCountryCode(longerNumber);

    const bool shorterNumberIsShortCode = isShortCode(shorterNumber, country);
    const bool longerNumberIsShortCode = isShortCode(longerNumber, country);

    if ((shorterNumberIsShortCode && !longerNumberIsShortCode) || (!shorterNumberIsShortCode && longerNumberIsShortCode)) {
        // If only one of the numbers is a short code, they clearly do not match
        return false;
    }

    bool matchingPhoneNumber = longerNumber.endsWith(shorterNumber);

    return matchingPhoneNumber;
}

bool SmsHelper::isPhoneNumberMatch(const QString& phone1, const QString& phone2)
{
    const QString& canonicalPhone1 = canonicalizePhoneNumber(phone1);
    const QString& canonicalPhone2 = canonicalizePhoneNumber(phone2);

    return isPhoneNumberMatchCanonicalized(canonicalPhone1, canonicalPhone2);
}

bool SmsHelper::isShortCode(const QString& phoneNumber, const SmsHelper::CountryCode& country)
{
    // Regardless of which country this number belongs to, a number of length less than 6 is a "short code"
    if (phoneNumber.length() <= 6) {
        return true;
    }
    if (country == CountryCode::Australia && phoneNumber.length() == 8 && phoneNumber.startsWith(QStringLiteral("19"))) {
        return true;
    }
    if (country == CountryCode::CzechRepublic && phoneNumber.length() <= 9) {
        // This entry of the Wikipedia article is fairly poorly written, so it is not clear whether a
        // short code with length 7 should start with a 9. Leave it like this for now, upgrade as
        // we get more information
        return true;
    }
    return false;
}

SmsHelper::CountryCode SmsHelper::determineCountryCode(const QString& canonicalNumber)
{
    // This is going to fall apart if someone has not entered a country code into their contact book
    // or if Android decides it can't be bothered to report the country code, but probably we will
    // be fine anyway
    if (canonicalNumber.startsWith(QStringLiteral("41"))) {
        return CountryCode::Australia;
    }
    if (canonicalNumber.startsWith(QStringLiteral("420"))) {
        return CountryCode::CzechRepublic;
    }

    // The only countries I care about for the current implementation are Australia and CzechRepublic
    // If we need to deal with further countries, we should probably find a library
    return CountryCode::Other;
}

QString SmsHelper::canonicalizePhoneNumber(const QString& phoneNumber)
{
    QString toReturn(phoneNumber);
    toReturn = toReturn.remove(QStringLiteral(" "));
    toReturn = toReturn.remove(QStringLiteral("-"));
    toReturn = toReturn.remove(QStringLiteral("("));
    toReturn = toReturn.remove(QStringLiteral(")"));
    toReturn = toReturn.remove(QStringLiteral("+"));
    toReturn = toReturn.remove(QRegularExpression(QStringLiteral("^0*"))); // Strip leading zeroes

    if (toReturn.length() == 0) {
        // If we have stripped away everything, assume this is a special number (and already canonicalized)
        return phoneNumber;
    }
    return toReturn;
}

class PersonsCache : public QObject {
public:
    PersonsCache() {
        connect(&m_people, &QAbstractItemModel::rowsRemoved, this, [this] (const QModelIndex &parent, int first, int last) {
            if (parent.isValid())
                return;
            for (int i=first; i<=last; ++i) {
                const QString& uri = m_people.get(i, KPeople::PersonsModel::PersonUriRole).toString();
                m_personDataCache.remove(uri);
            }
        });
    }

    QSharedPointer<KPeople::PersonData> personAt(int rowIndex) {
        const QString& uri = m_people.get(rowIndex, KPeople::PersonsModel::PersonUriRole).toString();
        auto& person = m_personDataCache[uri];
        if (!person)
            person.reset(new KPeople::PersonData(uri));
        return person;
    }

    int count() const {
        return m_people.rowCount();
    }

private:
    KPeople::PersonsModel m_people;
    QHash<QString, QSharedPointer<KPeople::PersonData>> m_personDataCache;
};

QSharedPointer<KPeople::PersonData> SmsHelper::lookupPersonByAddress(const QString& address)
{
    static PersonsCache s_cache;

    const QString& canonicalAddress = SmsHelper::canonicalizePhoneNumber(address);
    int rowIndex = 0;
    for (rowIndex = 0; rowIndex < s_cache.count(); rowIndex++) {
        const auto person = s_cache.personAt(rowIndex);

        const QStringList& allEmails = person->allEmails();
        for (const QString& email : allEmails) {
            // Although we are nominally an SMS messaging app, it is possible to send messages to phone numbers using email -> sms bridges
            if (address == email) {
                return person;
            }
        }

        // TODO: Either upgrade KPeople with an allPhoneNumbers method
        const QVariantList allPhoneNumbers = person->contactCustomProperty(QStringLiteral("all-phoneNumber")).toList();
        for (const QVariant& rawPhoneNumber : allPhoneNumbers) {
            const QString& phoneNumber = SmsHelper::canonicalizePhoneNumber(rawPhoneNumber.toString());
            bool matchingPhoneNumber = SmsHelper::isPhoneNumberMatchCanonicalized(canonicalAddress, phoneNumber);

            if (matchingPhoneNumber) {
                //qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Matched" << address << "to" << person->name();
                return person;
            }
        }
    }

    return nullptr;
}

QIcon SmsHelper::combineIcons(const QList<QPixmap>& icons) {
    QIcon icon;
    if (icons.size() == 0) {
        // We have no icon :(
        // Once we are using the generic icon from KPeople for unknown contacts, this should never happen
    } else if (icons.size() == 1) {
        icon = icons.first();
    } else {
        // Cook an icon by combining the available icons
        // Barring better information, use the size of the first icon as the size for the final icon
        QSize size = icons.first().size();
        QPixmap canvas(size);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);

        QSize halfSize = size / 2;

        QRect topLeftQuadrant(QPoint(0, 0), halfSize);
        QRect topRightQuadrant(topLeftQuadrant.topRight(), halfSize);
        QRect bottomLeftQuadrant(topLeftQuadrant.bottomLeft(), halfSize);
        QRect bottomRightQuadrant(topLeftQuadrant.bottomRight(), halfSize);

        if (icons.size() == 2) {
            painter.drawPixmap(topLeftQuadrant, icons[0]);
            painter.drawPixmap(bottomRightQuadrant, icons[1]);
        } else if (icons.size() == 3) {
            QRect topMiddle(QPoint(halfSize.width() / 2, 0), halfSize);
            painter.drawPixmap(topMiddle, icons[0]);
            painter.drawPixmap(bottomLeftQuadrant, icons[1]);
            painter.drawPixmap(bottomRightQuadrant, icons[2]);
        } else {
            // Four or more
            painter.drawPixmap(topLeftQuadrant, icons[0]);
            painter.drawPixmap(topRightQuadrant, icons[1]);
            painter.drawPixmap(bottomLeftQuadrant, icons[2]);
            painter.drawPixmap(bottomRightQuadrant, icons[3]);
        }

        icon = canvas;
    }
    return icon;
}

QString SmsHelper::getTitleForAddresses(const QList<ConversationAddress>& addresses) {
    QStringList titleParts;
    for (const ConversationAddress& address : addresses) {
        const auto personData = SmsHelper::lookupPersonByAddress(address.address());

        if (personData) {
            titleParts.append(personData->name());
        } else {
            titleParts.append(address.address());
        }
    }

    // It might be nice to alphabetize before combining so that the names don't move around randomly
    // (based on how the data came to us from Android)
    return titleParts.join(QLatin1String(", "));
}

QIcon SmsHelper::getIconForAddresses(const QList<ConversationAddress>& addresses) {
    QList<QPixmap> icons;
    for (const ConversationAddress& address : addresses) {
        const auto personData = SmsHelper::lookupPersonByAddress(address.address());

        if (personData) {
            icons.append(personData->photo());
        } else {
            static QString dummyAvatar = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/kpeople/dummy_avatar.png"));
            icons.append(QPixmap(dummyAvatar));
        }
    }

    // It might be nice to alphabetize by contact before combining so that the pictures don't move
    // around randomly (based on how the data came to us from Android)
    return combineIcons(icons);
}
