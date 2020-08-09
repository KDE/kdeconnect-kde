/**
 * Copyright (C) 2020 Aniket Kumar <anikketkumar786@gmail.com>
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
    AttachmentInfo(const Attachment& attachment);

    qint64 partID() const { return m_partID; }
    QString mimeType() const { return m_mimeType; }
    QString uniqueIdentifier() const { return m_uniqueIdentifier; }

private:
    qint64 m_partID;
    QString m_mimeType;
    QString m_uniqueIdentifier;
};

#endif // ATTACHMENTINFO_H
