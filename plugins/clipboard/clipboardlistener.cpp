/**
 * SPDX-FileCopyrightText: 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"
#include <memory>
#include <QDebug>

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
    : m_manager(new DataControlDeviceManager)
{
    connect(m_manager.get(), &DataControlDeviceManager::activeChanged, this, [this]() {
        if (m_manager->isActive()) {
            QPlatformNativeInterface *native = qApp->platformNativeInterface();
            if (!native) {
                return;
            }
            auto seat = static_cast<struct ::wl_seat *>(native->nativeResourceForIntegration("wl_seat"));
            if (!seat) {
                return;
            }

            m_device.reset(new DataControlDevice(m_manager->get_data_device(seat)));

            connect(m_device.get(), &DataControlDevice::receivedSelectionChanged, this, [this] {
                refresh(m_device->receivedSelection());
            });
            connect(m_device.get(), &DataControlDevice::selectionChanged, this, [this] {
                refresh(m_device->selection());
            });
        } else {
            m_device.reset();
        }
    });
}

void WaylandClipboardListener::setText(const QString& content)
{
    if (!m_device) {
        return;
    }

    refreshContent(content);
    auto mime = new QMimeData;
    mime->setText(content);
    auto source = std::make_unique<DataControlSource>(m_manager->create_data_source(), mime);
    mime->setParent(source.get());
    m_device->setSelection(std::move(source));
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
