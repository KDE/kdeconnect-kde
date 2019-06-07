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

#include <QString>
#include <QRegularExpression>
#include <QLoggingCategory>

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
    if (country == CountryCode::Australia && phoneNumber.length() == 8 && phoneNumber.startsWith("19")) {
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
    if (canonicalNumber.startsWith("41")) {
        return CountryCode::Australia;
    }
    if (canonicalNumber.startsWith("420")) {
        return CountryCode::CzechRepublic;
    }

    // The only countries I care about for the current implementation are Australia and CzechRepublic
    // If we need to deal with further countries, we should probably find a library
    return CountryCode::Other;
}

QString SmsHelper::canonicalizePhoneNumber(const QString& phoneNumber)
{
    QString toReturn(phoneNumber);
    toReturn = toReturn.remove(' ');
    toReturn = toReturn.remove('-');
    toReturn = toReturn.remove('(');
    toReturn = toReturn.remove(')');
    toReturn = toReturn.remove('+');
    toReturn = toReturn.remove(QRegularExpression("^0*")); // Strip leading zeroes

    if (toReturn.length() == 0) {
        // If we have stripped away everything, assume this is a special number (and already canonicalized)
        return phoneNumber;
    }
    return toReturn;
}
