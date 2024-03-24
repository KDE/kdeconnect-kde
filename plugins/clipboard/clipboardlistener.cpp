/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"

#include <KSystemClipboard>

#include <QDateTime>
#include <QMimeData>

QString ClipboardListener::currentContent()
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

void ClipboardListener::refreshContent(const QString &content, ClipboardListener::ClipboardContentType contentType)
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

    const QString content = clipboard->text(QClipboard::Clipboard);
    if ((content.isEmpty() || content == m_currentContent) && contentType == m_currentContentType) {
        return;
    }
    refreshContent(content, contentType);
    Q_EMIT clipboardChanged(content, contentType);
}

void ClipboardListener::setText(const QString &content)
{
    refreshContent(content, ClipboardListener::ClipboardContentTypeUnknown);
    auto mime = new QMimeData;
    mime->setText(content);
    clipboard->setMimeData(mime, QClipboard::Clipboard);
}

#include "moc_clipboardlistener.cpp"
