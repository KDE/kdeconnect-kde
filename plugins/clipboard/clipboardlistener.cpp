/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"

ClipboardListener::ClipboardListener()
    : clipboard(QGuiApplication::clipboard())
{
#ifdef Q_OS_MAC
    connect(&m_clipboardMonitorTimer, &QTimer::timeout, this, [this](){ updateClipboard(QClipboard::Clipboard); });
    m_clipboardMonitorTimer.start(1000);    // Refresh 1s
#endif
    connect(clipboard, &QClipboard::changed, this, &ClipboardListener::updateClipboard);
}

void ClipboardListener::updateClipboard(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard) {
        return;
    }

    QString content = clipboard->text();

    if (content == m_currentContent) {
        return;
    }
    m_updateTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_currentContent = content;

    Q_EMIT clipboardChanged(content);
}

QString ClipboardListener::currentContent()
{
    return m_currentContent;
}

qint64 ClipboardListener::updateTimestamp(){

    return m_updateTimestamp;
}

void ClipboardListener::setText(const QString& content)
{
    m_updateTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_currentContent = content;
    clipboard->setText(content);
}
