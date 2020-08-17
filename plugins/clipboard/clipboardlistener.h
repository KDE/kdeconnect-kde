/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CLIPBOARDLISTENER_H
#define CLIPBOARDLISTENER_H

#include <QDateTime>
#include <QTimer>
#include <QObject>
#include <QClipboard>
#include <QGuiApplication>

/**
 * Wrapper around QClipboard, which emits clipboardChanged only when it really changed
 */
class ClipboardListener : public QObject
{
    Q_OBJECT

private:
    ClipboardListener();
    QString m_currentContent;
    qint64 m_updateTimestamp = 0;
    QClipboard* clipboard;
#ifdef Q_OS_MAC
    QTimer m_clipboardMonitorTimer;
#endif

public:

    static ClipboardListener* instance()
    {
        static ClipboardListener* me = nullptr;
        if (!me) {
            me = new ClipboardListener();
        }
        return me;
    }

    void updateClipboard(QClipboard::Mode mode);

    void setText(const QString& content);

    QString currentContent();
    qint64 updateTimestamp();

Q_SIGNALS:
    void clipboardChanged(const QString& content);
};

#endif
