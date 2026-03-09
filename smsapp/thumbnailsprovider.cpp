/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "thumbnailsprovider.h"

#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QQmlContext>

ThumbnailsProvider::ThumbnailsProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

std::optional<ThumbnailsProvider *> ThumbnailsProvider::getInContextForObject(const QObject *obj)
{
    auto engine = QQmlEngine::contextForObject(obj)->engine();
    if (!engine) {
        qCritical() << "ThumbnailsProvider::getInContextForObject: No QML engine available for " << obj;
        return {};
    }
    auto imageProvider = engine->imageProvider(QStringLiteral("thumbnailsProvider"));
    if (!imageProvider) {
        qCritical() << "ThumbnailsProvider::getInContextForObject: Could not find thumbnailsProvider in QML engine";
        return {};
    }
    return {dynamic_cast<ThumbnailsProvider *>(imageProvider)};
}

QImage ThumbnailsProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (m_thumbnails.contains(id)) {
        auto const &icon = m_thumbnails.value(id);
        // Try to find a pixmap matching the desired icon size!
        QPixmap sizedPixmap = icon.pixmap(requestedSize);
        if (!sizedPixmap) {
            // No icon sifficiently small was found, let's give the first one
            sizedPixmap = icon.pixmap(icon.availableSizes().first());
        }
        // Inform the caller of the image size we're giving them!
        *size = sizedPixmap.size();
        return sizedPixmap.toImage();
    }

    return QImage();
}

void ThumbnailsProvider::addIcon(const QString &id, const QIcon &icon)
{
    m_thumbnails.insert(id, icon);
}

void ThumbnailsProvider::clear()
{
    m_thumbnails.clear();
}
