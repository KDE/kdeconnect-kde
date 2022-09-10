/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ATTACHMENTINFO_H
#define ATTACHMENTINFO_H

#include "conversationmessage.h"

/**
 * This class is a compressed version of Attachment class
 * as it will be exposed to the QML in a list object
 */
class AttachmentInfo
{
    Q_GADGET
    Q_PROPERTY(qint64 partID READ partID)
    Q_PROPERTY(QString mimeType READ mimeType)
    Q_PROPERTY(QString uniqueIdentifier READ uniqueIdentifier)

public:
    AttachmentInfo();
    AttachmentInfo(const Attachment &attachment);

    qint64 partID() const
    {
        return m_partID;
    }
    QString mimeType() const
    {
        return m_mimeType;
    }
    QString uniqueIdentifier() const
    {
        return m_uniqueIdentifier;
    }

private:
    qint64 m_partID;
    QString m_mimeType;
    QString m_uniqueIdentifier;
};

#endif // ATTACHMENTINFO_H
