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

#include "notifyingapplicationmodel.h"

#include <algorithm>

#include <QString>
#include <QIcon>
#include <QDebug>

#include <KLocalizedString>

//#include "modeltest.h"

NotifyingApplicationModel::NotifyingApplicationModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

NotifyingApplicationModel::~NotifyingApplicationModel()
{
}

QVector<NotifyingApplication> NotifyingApplicationModel::apps()
{
    return m_apps;
}

void NotifyingApplicationModel::appendApp(const NotifyingApplication& app)
{
    if (app.name.isEmpty() || apps().contains(app))
        return;
    beginInsertRows(QModelIndex(), m_apps.size(), m_apps.size());
    m_apps.append(app);
    endInsertRows();
}

bool NotifyingApplicationModel::containsApp(const QString& name) const
{
    for (const auto& a: m_apps)
        if (a.name == name)
            return true;
    return false;
}

Qt::ItemFlags NotifyingApplicationModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    if (index.isValid() && index.row() >= 0 && index.row() < m_apps.size() &&
            index.column() < 3)
    {
        if (index.column() == 0)
             flags |= Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
        else if (index.column() == 2) {
            if (m_apps[index.row()].active)
                flags |= Qt::ItemIsEditable;
            else
                flags ^= Qt::ItemIsEnabled;
        }
        else if (index.column() == 1) {
            if (!m_apps[index.row()].active)
                flags ^= Qt::ItemIsEnabled;
        }
    }
    return flags;
}

void NotifyingApplicationModel::clearApplications()
{
    if (!m_apps.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_apps.size() - 1);
        m_apps.clear();
        endRemoveRows();
    }
}

QVariant NotifyingApplicationModel::data(const QModelIndex& index, int role) const
{
    Q_UNUSED(role);
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_apps.size()
        || index.column() > 3)
    {
        return QVariant();
    }

    switch (role) {
        case Qt::TextAlignmentRole: {
            if (index.column() == 0)
                return int(Qt::AlignCenter | Qt::AlignVCenter );
            else
                return int(Qt::AlignLeft | Qt::AlignVCenter );
            break;
        }
        case Qt::DisplayRole: {
            if (index.column() == 1)
                return m_apps[index.row()].name;
            else if (index.column() == 0)
                return QVariant();//m_apps[index.row()].active;
            else if (index.column() == 2)
                return m_apps[index.row()].blacklistExpression.pattern();
            else
                return QVariant();
            break;
        }
        case Qt::DecorationRole: {
            if (index.column() == 1)
                return QIcon::fromTheme(m_apps[index.row()].icon, QIcon::fromTheme(QStringLiteral("application-x-executable")));
            else
                return QVariant();
            break;
        }
        case Qt::EditRole: {
            if (index.column() == 0)
                return m_apps[index.row()].active ? Qt::Checked : Qt::Unchecked;
            else if (index.column() == 2)
                return m_apps[index.row()].blacklistExpression.pattern();
            else
                return QVariant();
            break;
        }
        case Qt::CheckStateRole: {
            if (index.column() == 0)
                return m_apps[index.row()].active ? Qt::Checked : Qt::Unchecked;
            else
                return QVariant();
            break;
        }
    }
    return QVariant();
}

bool NotifyingApplicationModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() ||
            (index.column() != 0 && index.column() != 2) ||
            index.row() < 0 || index.row() >= m_apps.size())
        return false;

    bool res = false;
    QModelIndex bottomRight = createIndex(index.row(), index.column());
    switch (role) {
        case Qt::CheckStateRole: {
            if (index.column() == 0) {
                m_apps[index.row()].active = ((Qt::CheckState)value.toInt() == Qt::Checked);
                bottomRight = createIndex(index.row(), index.column() + 1);
                res = true;
            }
            break;
        }
        case Qt::EditRole: {
            if (index.column() == 2) {
                m_apps[index.row()].blacklistExpression.setPattern(value.toString());
                res = true;
            }
        }
    }
    if (res) {
        Q_EMIT dataChanged(index, bottomRight);
        Q_EMIT applicationsChanged();  // -> notify config that we need to save
    }
    return res;
}

void NotifyingApplicationModel::sort(int column, Qt::SortOrder order)
{
    if (column != 1)
        return;

    if (order == Qt::AscendingOrder)
        std::sort(m_apps.begin(), m_apps.end(),
                  [](const NotifyingApplication& a, const NotifyingApplication& b) {
                     return (a.name.compare(b.name,  Qt::CaseInsensitive) < 1);
                  });
    else
        std::sort(m_apps.begin(), m_apps.end(),
                  [](const NotifyingApplication& a, const NotifyingApplication& b) {
                     return (b.name.compare(a.name,  Qt::CaseInsensitive) < 1);
                  });
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(m_apps.size(), 2));
}

QVariant NotifyingApplicationModel::headerData(int section, Qt::Orientation /*orientation*/,
                                          int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (section == 1)
                return i18n("Name");
            else if (section == 0)
                return QVariant(); //i18n("Sync");
            else
                return i18n("Blacklisted");
        }
        case Qt::ToolTipRole: {
            if (section == 1)
                return i18n("Name of a notifying application.");
            else if (section == 0)
                return i18n("Synchronize notifications of an application?");
            else
                return i18n("Regular expression defining which notifications should not be sent.\nThis pattern is applied to the summary and, if selected above, the body of notifications.");
        }
    }
    return QVariant();
}

int NotifyingApplicationModel::columnCount(const QModelIndex&) const
{
    return 3;
}

int NotifyingApplicationModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }
    return m_apps.size();
}
