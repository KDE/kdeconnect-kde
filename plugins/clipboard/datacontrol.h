/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "qwayland-wlr-data-control-unstable-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QMimeData>
#include <QSharedPointer>

#include <qpa/qplatformnativeinterface.h>
#include <sys/select.h>
#include <memory>

class DataControlDeviceManager : public QWaylandClientExtensionTemplate<DataControlDeviceManager>, public QtWayland::zwlr_data_control_manager_v1
{
    Q_OBJECT
public:
    DataControlDeviceManager();
    ~DataControlDeviceManager();
};

class DataControlOffer : public QMimeData, public QtWayland::zwlr_data_control_offer_v1
{
    Q_OBJECT
public:
    DataControlOffer(struct ::zwlr_data_control_offer_v1 *id);
    ~DataControlOffer();

    QStringList formats() const override;
    bool hasFormat(const QString &format) const override;

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

class DataControlSource : public QObject, public QtWayland::zwlr_data_control_source_v1
{
    Q_OBJECT
public:
    DataControlSource(struct ::zwlr_data_control_source_v1 *id, const QMimeData *mime);
    DataControlSource();
    ~DataControlSource();

    const QMimeData *mimeData() const {
        return m_mimeData;
    }

Q_SIGNALS:
    void cancelled();

protected:
    void zwlr_data_control_source_v1_send(const QString &mime_type, int32_t fd) override;
    void zwlr_data_control_source_v1_cancelled() override;

private:
    const QMimeData *m_mimeData;
};

class DataControlDevice : public QObject, public QtWayland::zwlr_data_control_device_v1
{
    Q_OBJECT
public:
    DataControlDevice(struct ::zwlr_data_control_device_v1 *id);
    ~DataControlDevice();

    void setSelection(std::unique_ptr<DataControlSource> selection);
    const QMimeData *receivedSelection() const
    {
        return m_receivedSelection.get();
    }
    const QMimeData *selection() const
    {
        return m_selection ? m_selection->mimeData() : nullptr;
    }

Q_SIGNALS:
    void receivedSelectionChanged();
    void selectionChanged();

protected:
    void zwlr_data_control_device_v1_data_offer(struct ::zwlr_data_control_offer_v1 *id) override;
    void zwlr_data_control_device_v1_selection(struct ::zwlr_data_control_offer_v1 *id) override;

private:
    std::unique_ptr<DataControlSource> m_selection; // selection set locally
    QSharedPointer<DataControlOffer> m_receivedSelection; // latest selection set from externally to here
};
