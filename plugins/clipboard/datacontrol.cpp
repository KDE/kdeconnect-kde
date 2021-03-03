/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "datacontrol.h"
#include <QFile>
#include <QGuiApplication>
#include <QDebug>
#include <unistd.h>
#include <plugin_clipboard_debug.h>

DataControlDeviceManager::DataControlDeviceManager()
    : QWaylandClientExtensionTemplate<DataControlDeviceManager>(1)
{
}

DataControlDeviceManager::~DataControlDeviceManager()
{
    destroy();
}


DataControlOffer::DataControlOffer(::zwlr_data_control_offer_v1* id)
    : QtWayland::zwlr_data_control_offer_v1(id)
{
}

DataControlOffer::~DataControlOffer()
{
    destroy();
}

QStringList DataControlOffer::formats() const
{
    return m_receivedFormats;
}

bool DataControlOffer::hasFormat(const QString& format) const
{
    return m_receivedFormats.contains(format);
}

QVariant DataControlOffer::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    if (!hasFormat(mimeType)) {
        return QVariant();
    }
    Q_UNUSED(type);

    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        return QVariant();
    }

    auto t = const_cast<DataControlOffer *>(this);
    t->receive(mimeType, pipeFds[1]);

    close(pipeFds[1]);

    /*
     * Ideally we need to introduce a non-blocking QMimeData object
     * Or a non-blocking constructor to QMimeData with the mimetypes that are relevant
     *
     * However this isn't actually any worse than X.
     */

    QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
    auto display = static_cast<struct ::wl_display *>(native->nativeResourceForIntegration("wl_display"));
    wl_display_flush(display);

    QFile readPipe;
    if (readPipe.open(pipeFds[0], QIODevice::ReadOnly)) {
        QByteArray data;
        if (readData(pipeFds[0], data)) {
            return data;
        }
        close(pipeFds[0]);
    }
    return QVariant();
}

// reads data from a file descriptor with a timeout of 1 second
// true if data is read successfully
bool DataControlOffer::readData(int fd, QByteArray &data)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    Q_FOREVER {
        int ready = select(FD_SETSIZE, &readset, nullptr, nullptr, &timeout);
        if (ready < 0) {
            qCWarning(KDECONNECT_PLUGIN_CLIPBOARD) << "DataControlOffer: select() failed";
            return false;
        } else if (ready == 0) {
            qCWarning(KDECONNECT_PLUGIN_CLIPBOARD) << "DataControlOffer: timeout reading from pipe";
            return false;
        } else {
            char buf[4096];
            int n = read(fd, buf, sizeof buf);

            if (n < 0) {
                qCWarning(KDECONNECT_PLUGIN_CLIPBOARD) << "DataControlOffer: read() failed";
                return false;
            } else if (n == 0) {
                return true;
            } else if (n > 0) {
                data.append(buf, n);
            }
        }
    }
}

DataControlSource::~DataControlSource()
{
    destroy();
}

DataControlSource::DataControlSource(struct ::zwlr_data_control_source_v1 *id, const QMimeData *mimeData)
    : QtWayland::zwlr_data_control_source_v1(id)
    , m_mimeData(mimeData)
{
    for (const QString &format : mimeData->formats()) {
        offer(format);
    }
    if(mimeData->hasText()) {
        // ensure GTK applications get this mimetype to avoid them discarding the offer
        offer(QStringLiteral("text/plain;charset=utf-8"));
    }
}

void DataControlSource::zwlr_data_control_source_v1_send(const QString &mime_type, int32_t fd)
{
    QString send_mime_type = mime_type;
    if(send_mime_type == QStringLiteral("text/plain;charset=utf-8")) {
        // if we get a request on the fallback mime, send the data from the original mime type
        send_mime_type = QStringLiteral("text/plain");
    }

    QFile c;
    if (c.open(fd, QFile::WriteOnly, QFile::AutoCloseHandle)) {
        c.write(m_mimeData->data(send_mime_type));
        c.close();
    }
}

void DataControlSource::zwlr_data_control_source_v1_cancelled()
{
    Q_EMIT cancelled();
}

void DataControlDevice::zwlr_data_control_device_v1_data_offer(zwlr_data_control_offer_v1* id)
{
    new DataControlOffer(id);
    // this will become memory managed when we retrieve the selection event
    // a compositor calling data_offer without doing that would be a bug
}

void DataControlDevice::zwlr_data_control_device_v1_selection(zwlr_data_control_offer_v1* id)
{
    if (!id) {
        m_receivedSelection.reset();
    } else {
        auto deriv = QtWayland::zwlr_data_control_offer_v1::fromObject(id);
        auto offer = dynamic_cast<DataControlOffer *>(deriv); // dynamic because of the dual inheritance
        m_receivedSelection.reset(offer);
    }
    Q_EMIT receivedSelectionChanged();
}

void DataControlDevice::setSelection(std::unique_ptr<DataControlSource> selection)
{
    m_selection = std::move(selection);
    connect(m_selection.get(), &DataControlSource::cancelled, this, [this]() {
        m_selection.reset();
        Q_EMIT selectionChanged();
    });
    set_selection(m_selection->object());
    Q_EMIT selectionChanged();
}

DataControlDevice::DataControlDevice(::zwlr_data_control_device_v1* id)
    : QtWayland::zwlr_data_control_device_v1(id)
{
}

DataControlDevice::~DataControlDevice()
{
    destroy();
}
