/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"
#include <QDebug>
#include <QMimeData>

#include "datacontrol.h"

ClipboardListener::ClipboardListener()
{}

QString ClipboardListener::currentContent()
{
    return m_currentContent;
}

qint64 ClipboardListener::updateTimestamp(){

    return m_updateTimestamp;
}

ClipboardListener* ClipboardListener::instance()
{
    static ClipboardListener* me = nullptr;
    if (!me) {
        if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            me = new WaylandClipboardListener();
        } else {
            me = new QClipboardListener();
        }
    }
    return me;
}

void ClipboardListener::refreshContent(const QString& content)
{
    m_updateTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_currentContent = content;
}

QClipboardListener::QClipboardListener()
    : clipboard(QGuiApplication::clipboard())
{
#ifdef Q_OS_MAC
    connect(&m_clipboardMonitorTimer, &QTimer::timeout, this, [this](){ updateClipboard(QClipboard::Clipboard); });
    m_clipboardMonitorTimer.start(1000);    // Refresh 1s
#endif
    connect(clipboard, &QClipboard::changed, this, &QClipboardListener::updateClipboard);
}

void QClipboardListener::updateClipboard(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard) {
        return;
    }

    const QString content = clipboard->text();
    if (content == m_currentContent) {
        return;
    }
    refreshContent(content);
    Q_EMIT clipboardChanged(content);
}

void QClipboardListener::setText(const QString& content)
{
    refreshContent(content);
    clipboard->setText(content);
}

WaylandClipboardListener::WaylandClipboardListener()
    : m_dataControl(new DataControl(this))
{
    connect(m_dataControl, &DataControl::receivedSelectionChanged, this, [this] {
        refresh(m_dataControl->receivedSelection());
    });
    connect(m_dataControl, &DataControl::selectionChanged, this, [this] {
        refresh(m_dataControl->selection());
    });

}

void WaylandClipboardListener::setText(const QString& content)
{
    refreshContent(content);
    auto mime = new QMimeData;
    mime->setText(content);
    m_dataControl->setSelection(mime, true);
}

void WaylandClipboardListener::refresh(const QMimeData *mime)
{
    if (!mime || !mime->hasText()) {
        return;
    }

    const QString content = mime->text();
    if (content == m_currentContent) {
        return;
    }
    refreshContent(content);
    Q_EMIT clipboardChanged(content);
}
