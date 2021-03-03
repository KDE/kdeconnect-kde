/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "datacontrol.h"
#include "qwayland-wlr-data-control-unstable-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>
#include <qpa/qplatformnativeinterface.h>
#include <QVariant>
#include <unistd.h>

#include <plugin_clipboard_debug.h>
#include <sys/select.h>

class DataControlDeviceManager : public QWaylandClientExtensionTemplate<DataControlDeviceManager>, public QtWayland::zwlr_data_control_manager_v1
{
    Q_OBJECT
public:
    DataControlDeviceManager()
        : QWaylandClientExtensionTemplate<DataControlDeviceManager>(1)
    {
    }

    ~DataControlDeviceManager()
    {
        destroy();
    }
};

class DataControlOffer : public QMimeData, public QtWayland::zwlr_data_control_offer_v1
{
    Q_OBJECT
public:
    DataControlOffer(struct ::zwlr_data_control_offer_v1 *id)
        : QtWayland::zwlr_data_control_offer_v1(id)
    {
    }

    ~DataControlOffer()
    {
        destroy();
    }

    QStringList formats() const override
    {
        return m_receivedFormats;
    }

    bool hasFormat(const QString &format) const override
    {
        return m_receivedFormats.contains(format);
    }

protected:
    void zwlr_data_control_offer_v1_offer(const QString &mime_type) override
    {
        m_receivedFormats << mime_type;
    }

    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const override;

private:
    static bool readData(int fd, QByteArray &data);
    QStringList m_receivedFormats;
};

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

    QPlatformNativeInterface *native = qApp->platformNativeInterface();
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
                qWarning("DataControlOffer: read() failed");
                return false;
            } else if (n == 0) {
                return true;
            } else if (n > 0) {
                data.append(buf, n);
            }
        }
    }
}

class DataControlSource : public QObject, public QtWayland::zwlr_data_control_source_v1
{
    Q_OBJECT
public:
    DataControlSource(struct ::zwlr_data_control_source_v1 *id, QMimeData *mimeData);
    DataControlSource();
    ~DataControlSource()
    {
        destroy();
    }

    QMimeData *mimeData()
    {
        return m_mimeData;
    }

Q_SIGNALS:
    void cancelled();

protected:
    void zwlr_data_control_source_v1_send(const QString &mime_type, int32_t fd) override;
    void zwlr_data_control_source_v1_cancelled() override;

private:
    QMimeData *m_mimeData;
};

DataControlSource::DataControlSource(struct ::zwlr_data_control_source_v1 *id, QMimeData *mimeData)
    : QtWayland::zwlr_data_control_source_v1(id)
    , m_mimeData(mimeData)
{
    for (const QString &format : mimeData->formats()) {
        offer(format);
    }
    if(mimeData->hasText())
    {
        // ensure GTK applications get this mimetype to avoid them discarding the offer
        offer(QStringLiteral("text/plain;charset=utf-8"));
    }
}

void DataControlSource::zwlr_data_control_source_v1_send(const QString &mime_type, int32_t fd)
{
    QFile c;
    QString send_mime_type = mime_type;
    if(send_mime_type == QStringLiteral("text/plain;charset=utf-8")) {
        // if we get a request on the fallback mime, send the data from the original mime type
        send_mime_type = QStringLiteral("text/plain");
    }
    if (c.open(fd, QFile::WriteOnly, QFile::AutoCloseHandle)) {
        c.write(m_mimeData->data(send_mime_type));
        c.close();
    }
}

void DataControlSource::zwlr_data_control_source_v1_cancelled()
{
    Q_EMIT cancelled();
}

class DataControlDevice : public QObject, public QtWayland::zwlr_data_control_device_v1
{
    Q_OBJECT
public:
    DataControlDevice(struct ::zwlr_data_control_device_v1 *id)
        : QtWayland::zwlr_data_control_device_v1(id)
    {
    }

    ~DataControlDevice()
    {
        destroy();
    }

    void setSelection(std::unique_ptr<DataControlSource> selection);
    QMimeData *receivedSelection()
    {
        return m_receivedSelection.get();
    }
    QMimeData *selection()
    {
        return m_selection ? m_selection->mimeData() : nullptr;
    }

Q_SIGNALS:
    void receivedSelectionChanged();
    void selectionChanged();

protected:
    void zwlr_data_control_device_v1_data_offer(struct ::zwlr_data_control_offer_v1 *id) override
    {
        new DataControlOffer(id);
        // this will become memory managed when we retrieve the selection event
        // a compositor calling data_offer without doing that would be a bug
    }

    void zwlr_data_control_device_v1_selection(struct ::zwlr_data_control_offer_v1 *id) override
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

private:
    std::unique_ptr<DataControlSource> m_selection; // selection set locally
    std::unique_ptr<DataControlOffer> m_receivedSelection; // latest selection set from externally to here
};

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

class DataControlPrivate
{
public:
    DataControlPrivate()
        : m_manager(new DataControlDeviceManager)
    {}

    std::unique_ptr<DataControlDeviceManager> m_manager;
    std::unique_ptr<DataControlDevice> m_device;
};


DataControl::DataControl(QObject *parent)
    : QObject(parent)
    , d(new DataControlPrivate)
{
    connect(d->m_manager.get(), &DataControlDeviceManager::activeChanged, this, [this]() {
        if (d->m_manager->isActive()) {
            QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
            if (!native) {
                return;
            }
            auto seat = static_cast<struct ::wl_seat *>(native->nativeResourceForIntegration("wl_seat"));
            if (!seat) {
                return;
            }

            d->m_device.reset(new DataControlDevice(d->m_manager->get_data_device(seat)));

            connect(d->m_device.get(), &DataControlDevice::receivedSelectionChanged, this, &DataControl::receivedSelectionChanged);
            connect(d->m_device.get(), &DataControlDevice::selectionChanged, this, &DataControl::selectionChanged);
        } else {
            d->m_device.reset();
        }
    });
}

DataControl::~DataControl() = default;

QMimeData *DataControl::selection() const
{
    return d->m_device ? d->m_device->selection() : nullptr;
}

QMimeData * DataControl::receivedSelection() const
{
    return d->m_device ? d->m_device->receivedSelection() : nullptr;
}

void DataControl::setSelection(QMimeData* mime, bool ownMime)
{
    if (!d->m_device) {
        return;
    }

    auto source = std::make_unique<DataControlSource>(d->m_manager->create_data_source(), mime);
    if (ownMime) {
        mime->setParent(source.get());
    }
    d->m_device->setSelection(std::move(source));
}

void DataControl::clearSelection()
{
    if (!d->m_device) {
        return;
    }
    d->m_device->set_selection(nullptr);
}

void DataControl::clearPrimarySelection()
{
    if (!d->m_device) {
        return;
    }

    if (zwlr_data_control_device_v1_get_version(d->m_device->object()) >= ZWLR_DATA_CONTROL_DEVICE_V1_SET_PRIMARY_SELECTION_SINCE_VERSION) {
        d->m_device->set_primary_selection(nullptr);
    }
}

#include "datacontrol.moc"
