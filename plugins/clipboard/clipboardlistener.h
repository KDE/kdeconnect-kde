/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KJob>
#include <QClipboard>
#include <QObject>
#include <QPointer>
#include <QVariant>

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
        ClipboardContentTypeFile = 2
    };

protected:
    ClipboardListener();
    void refreshContent(const QVariant &content, ClipboardContentType contentType);
    QVariant m_currentContent;
    ClipboardContentType m_currentContentType;

private:
    qint64 m_updateTimestamp = 0;

public:
    static ClipboardListener *instance();

    void setText(const QString &content);
    void setFile(const KJob *job);

    QVariant currentContent();
    ClipboardContentType currentContentType();
    qint64 updateTimestamp();

Q_SIGNALS:
    void clipboardChanged(const QVariant &content, ClipboardContentType contentType);

private:
    void updateClipboard(QClipboard::Mode mode);

#ifdef Q_OS_MAC
    QTimer m_clipboardMonitorTimer;
#endif
    KSystemClipboard *clipboard;
};
