/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QClipboard>
#include <QObject>

#ifdef Q_OS_MAC
#include <QTimer>
#endif

class KSystemClipboard;

/**
 * Wrapper around QClipboard, which emits clipboardChanged only when it really changed
 */

class ClipboardListener : public QObject
{
    Q_OBJECT

public:
    enum ClipboardContentType {
        ClipboardContentTypeUnknown = 0,
        ClipboardContentTypePassword = 1,
    };

protected:
    ClipboardListener();
    void refreshContent(const QString &content, ClipboardContentType contentType);
    QString m_currentContent;
    ClipboardContentType m_currentContentType;

private:
    qint64 m_updateTimestamp = 0;

public:
    static ClipboardListener *instance();

    void setText(const QString &content);

    QString currentContent();
    ClipboardContentType currentContentType();
    qint64 updateTimestamp();

Q_SIGNALS:
    void clipboardChanged(const QString &content, ClipboardContentType contentType);

private:
    void updateClipboard(QClipboard::Mode mode);

#ifdef Q_OS_MAC
    QTimer m_clipboardMonitorTimer;
#endif
    KSystemClipboard *clipboard;
};
