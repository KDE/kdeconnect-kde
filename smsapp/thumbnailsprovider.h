/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef THUMBNAILSPROVIDER_H
#define THUMBNAILSPROVIDER_H

#include <QIcon>
#include <QQuickImageProvider>

#include <optional>

class ThumbnailsProvider : public QQuickImageProvider
{
public:
    ThumbnailsProvider();

    static std::optional<ThumbnailsProvider *> getInContextForObject(const QObject *obj);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    void addIcon(const QString &id, const QIcon &icon);

    void clear();

private:
    QHash<QString, QIcon> m_thumbnails;
};

#endif // THUMBNAILSPROVIDER_H
