/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "thumbnailsprovider.h"

ThumbnailsProvider::ThumbnailsProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ThumbnailsProvider::requestImage(const QString &id, QSize * /*size*/, const QSize & /*requestedSize*/)
{
    if (m_thumbnails.contains(id)) {
        return m_thumbnails.value(id);
    }

    return QImage();
}

void ThumbnailsProvider::addImage(const QString &id, const QImage &image)
{
    m_thumbnails.insert(id, image);
}

void ThumbnailsProvider::clear()
{
    m_thumbnails.clear();
}
