/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CLIPBOARDLISTENER_H
#define CLIPBOARDLISTENER_H

#include <QObject>
#include <QClipboard>

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

protected:
    ClipboardListener();
    void refreshContent(const QString &content);
    QString m_currentContent;

private:
    qint64 m_updateTimestamp = 0;

public:
    static ClipboardListener* instance();

    void setText(const QString& content);

    QString currentContent();
    qint64 updateTimestamp();

Q_SIGNALS:
    void clipboardChanged(const QString& content);

private:
    void updateClipboard(QClipboard::Mode mode);

#ifdef Q_OS_MAC
    QTimer *m_clipboardMonitorTimer;
#endif
    KSystemClipboard *clipboard;
};

#endif
