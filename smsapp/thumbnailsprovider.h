/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef THUMBNAILSPROVIDER_H
#define THUMBNAILSPROVIDER_H

#include <QQuickImageProvider>

class ThumbnailsProvider : public QQuickImageProvider
{
public:
    ThumbnailsProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    void addImage(const QString &id, const QImage &image);

    void clear();

private:
    QHash<QString, QImage> m_thumbnails;
};

#endif // THUMBNAILSPROVIDER_H
