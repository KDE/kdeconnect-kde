/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once
#include <QObject>
#include <QScopedPointer>
#include <QClipboard>
#include <memory>

class DataControlDevice;
class DataControlDeviceManager;
class QMimeData;

class DataControl : public QObject
{
    Q_OBJECT
public:
    DataControl(QObject *parent = nullptr);
    ~DataControl() override;

    const QMimeData *mimeData(QClipboard::Mode mode) const;
    void setMimeData(QMimeData *mime, QClipboard::Mode mode);
    void clear(QClipboard::Mode mode);

Q_SIGNALS:
    void changed(QClipboard::Mode mode);

private:
    std::unique_ptr<DataControlDeviceManager> m_manager;
    std::unique_ptr<DataControlDevice> m_device;
};
