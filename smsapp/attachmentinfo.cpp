/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "attachmentinfo.h"

AttachmentInfo::AttachmentInfo()
{
}

AttachmentInfo::AttachmentInfo(const Attachment &attachment)
    : m_partID(attachment.partID())
    , m_mimeType(attachment.mimeType())
    , m_uniqueIdentifier(attachment.uniqueIdentifier())
{
}

#include "moc_attachmentinfo.cpp"
