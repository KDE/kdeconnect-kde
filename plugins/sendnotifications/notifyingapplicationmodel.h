/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NOTIFYINGAPPLICATIONMODEL_H
#define NOTIFYINGAPPLICATIONMODEL_H

#include <QAbstractTableModel>

#include "notifyingapplication.h"

class NotifyingApplicationModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NotifyingApplicationModel(QObject* parent = nullptr);
    ~NotifyingApplicationModel() override;

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QVector<NotifyingApplication> apps();
    void clearApplications();
    void appendApp(const NotifyingApplication& app);
    bool containsApp(const QString& name) const;

Q_SIGNALS:
    void applicationsChanged();

private:
    QVector<NotifyingApplication> m_apps;
};

#endif // NOTIFYINGAPPLICATIONMODEL_H
