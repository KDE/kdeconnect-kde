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

#ifndef THUMBNAILSPROVIDER_H
#define THUMBNAILSPROVIDER_H

#include <QQuickImageProvider>

class ThumbnailsProvider : public QQuickImageProvider
{
public:
    ThumbnailsProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    void addImage(const QString& id, const QImage& image);

    void clear();

private:
    QHash<QString, QImage> m_thumbnails;
};

#endif // THUMBNAILSPROVIDER_H
