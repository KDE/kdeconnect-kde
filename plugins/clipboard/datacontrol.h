/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once
#include <QObject>
#include <QScopedPointer>

class DataControlPrivate;
class QMimeData;

class DataControl : public QObject
{
    Q_OBJECT
public:
    DataControl(QObject *parent = nullptr);
    ~DataControl() override;

    QMimeData* selection() const;
    QMimeData* receivedSelection() const;

    void clearSelection();
    void clearPrimarySelection();

    void setSelection(QMimeData *mime, bool ownMime);

Q_SIGNALS:
    void receivedSelectionChanged();
    void selectionChanged();

private:
    QScopedPointer<DataControlPrivate> d;
};
