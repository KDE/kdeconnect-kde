/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QAbstractTableModel>

#include "notifyingapplication.h"

class NotifyingApplicationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NotifyingApplicationModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QVector<NotifyingApplication> apps();
    void clearApplications();
    void appendApp(const NotifyingApplication &app);
    bool containsApp(const QString &name) const;

Q_SIGNALS:
    void applicationsChanged();

private:
    QVector<NotifyingApplication> m_apps;
};
