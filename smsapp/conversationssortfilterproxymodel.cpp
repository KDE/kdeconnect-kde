/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conversationssortfilterproxymodel.h"
#include "conversationlistmodel.h"

#include <QString>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_SORT_FILTER_PROXY_MODEL, "kdeconnect.sms.conversations_sort_filter_proxy")

#define INVALID_THREAD_ID -1

ConversationsSortFilterProxyModel::ConversationsSortFilterProxyModel()
{
    setFilterRole(ConversationListModel::DateRole);
}

ConversationsSortFilterProxyModel::~ConversationsSortFilterProxyModel(){}

void ConversationsSortFilterProxyModel::setOurFilterRole(int role)
{
    setFilterRole(role);
}

bool ConversationsSortFilterProxyModel::lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const
{
    QVariant leftDataTimeStamp = sourceModel()->data(leftIndex, ConversationListModel::DateRole);
    QVariant rightDataTimeStamp = sourceModel()->data(rightIndex, ConversationListModel::DateRole);

    if (leftDataTimeStamp == rightDataTimeStamp) {
        QVariant leftDataName = sourceModel()->data(leftIndex, Qt::DisplayRole);
        QVariant rightDataName = sourceModel()->data(rightIndex, Qt::DisplayRole);
        return leftDataName.toString().toLower() > rightDataName.toString().toLower();
    }
    return leftDataTimeStamp < rightDataTimeStamp;
}

bool ConversationsSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (filterRole() == Qt::DisplayRole) {
       return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
    return sourceModel()->data(index, ConversationListModel::DateRole) != INVALID_THREAD_ID;
}
