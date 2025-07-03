/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"
#include "filetransferjob.h"
#include "plugin_clipboard_debug.h"

#include <KSystemClipboard>

#include <QDateTime>
#include <QImage>
#include <QMimeData>
#include <QPixmap>
#include <QStringLiteral>
#include <QUrl>
#include <QVariant>
#include <QtCore>
#include <QtWidgets/qapplication.h>

QVariant ClipboardListener::currentContent()
{
    return m_currentContent;
}

ClipboardListener::ClipboardContentType ClipboardListener::currentContentType()
{
    return m_currentContentType;
}

qint64 ClipboardListener::updateTimestamp()
{
    return m_updateTimestamp;
}

ClipboardListener *ClipboardListener::instance()
{
    static ClipboardListener *me = nullptr;
    if (!me) {
        me = new ClipboardListener();
    }
    return me;
}

void ClipboardListener::refreshContent(const QVariant &content, ClipboardListener::ClipboardContentType contentType)
{
    m_updateTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_currentContent = content;
    m_currentContentType = contentType;
}

ClipboardListener::ClipboardListener()
    : clipboard(KSystemClipboard::instance())
{
#ifdef Q_OS_MAC
    connect(&m_clipboardMonitorTimer, &QTimer::timeout, this, [this]() {
        updateClipboard(QClipboard::Clipboard);
    });
    m_clipboardMonitorTimer.start(1000); // Refresh 1s
#endif
    connect(clipboard, &KSystemClipboard::changed, this, &ClipboardListener::updateClipboard);
}

void ClipboardListener::updateClipboard(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard) {
        return;
    }

    ClipboardListener::ClipboardContentType contentType = ClipboardListener::ClipboardContentTypeUnknown;
    if (clipboard->mimeData(mode) && clipboard->mimeData(mode)->data(QStringLiteral("x-kde-passwordManagerHint")) == QByteArrayLiteral("secret")) {
        contentType = ClipboardListener::ClipboardContentTypePassword;
    }

    const QMimeData *currentMime = clipboard->mimeData(QClipboard::Clipboard);
    if (currentMime && currentMime->hasUrls()) {
        const QList<QUrl> &urls = currentMime->urls();
        // Even though the mime data announces URLs, fetching them might still fail.
        if (urls.isEmpty()) {
            return;
        }

        const QUrl url = urls.first();
        if (url == m_currentContent) {
            return;
        }

        refreshContent(url, ClipboardListener::ClipboardContentTypeFile);
        Q_EMIT clipboardChanged(url, ClipboardListener::ClipboardContentTypeFile);
    } else if (currentMime && currentMime->hasImage()) {
        const QImage imageData = qvariant_cast<QImage>(currentMime->imageData());

        if (imageData == m_currentContent) {
            return;
        }

        refreshContent(imageData, ClipboardListener::ClipboardContentTypeFile);
        Q_EMIT clipboardChanged(imageData, ClipboardListener::ClipboardContentTypeFile);
    } else {
        const QString content = clipboard->text(QClipboard::Clipboard);
        if ((content.isEmpty() || content == m_currentContent) && contentType == m_currentContentType) {
            return;
        }

        refreshContent(content, contentType);
        Q_EMIT clipboardChanged(content, contentType);
    }
}

void ClipboardListener::setText(const QString &content)
{
    refreshContent(content, ClipboardListener::ClipboardContentTypeUnknown);
    auto mime = new QMimeData;
    mime->setText(content);
    clipboard->setMimeData(mime, QClipboard::Clipboard);
}

void ClipboardListener::setFile(const KJob *job)
{
    const auto *ftjob = qobject_cast<const FileTransferJob *>(job);
    if (ftjob && !job->error()) {
        QUrl url = ftjob->destination();

        if (url.isEmpty()) {
            qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "Could not open the image for clipboard";
            return;
        }

        refreshContent(url, ClipboardContentType::ClipboardContentTypeFile);

        auto mime = new QMimeData;
        mime->setUrls({url});
        clipboard->setMimeData(mime, QClipboard::Clipboard);
    } else {
        qCDebug(KDECONNECT_PLUGIN_CLIPBOARD) << "Could not receive the image for clipboard";
    }
}

#include "moc_clipboardlistener.cpp"
