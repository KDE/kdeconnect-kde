/**
 * SPDX-FileCopyrightText: 2020 Jiří Wolker <woljiri@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CHARCOUNT_H
#define CHARCOUNT_H

class SmsCharCount
{
public:
    /**
     * Number of octets in current message.
     */
    qint32 octets;

    /**
     * Bits per character (7, 8 or 16).
     */
    qint32 bitsPerChar;

    /**
     * Number of chars remaining in current SMS.
     */
    qint32 remaining;

    /**
     * Count of SMSes in concatenated SMS.
     */
    qint32 messages;

    SmsCharCount() {};
    ~SmsCharCount() {};
};

#endif // CHARCOUNT_H
