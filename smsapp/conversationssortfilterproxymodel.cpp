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
#include "smshelper.h"

#include <QString>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_SORT_FILTER_PROXY_MODEL, "kdeconnect.sms.conversations_sort_filter_proxy")

#define INVALID_THREAD_ID -1

ConversationsSortFilterProxyModel::ConversationsSortFilterProxyModel()
{
    setFilterRole(ConversationListModel::ConversationIdRole);
}

ConversationsSortFilterProxyModel::~ConversationsSortFilterProxyModel() {}

void ConversationsSortFilterProxyModel::setConversationsFilterRole(int role)
{
    setFilterRole(role);
}

bool ConversationsSortFilterProxyModel::lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const
{
    // This if block checks for multitarget conversations and sorts it at bottom of the list when the filtring is done on the basis of SenderRole
    // This keeps the individual contacts with matching address at the top of the list
    if (filterRole() == ConversationListModel::AddressesRole) {
        const bool isLeftMultitarget = sourceModel()->data(leftIndex, ConversationListModel::MultitargetRole).toBool();
        const bool isRightMultitarget = sourceModel()->data(rightIndex, ConversationListModel::MultitargetRole).toBool();
        if (isLeftMultitarget && !isRightMultitarget) {
            return true;
        }
        if (!isLeftMultitarget && isRightMultitarget) {
            return true;
        }
    }

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

    if (filterRole() == ConversationListModel::ConversationIdRole) {
        return sourceModel()->data(index, ConversationListModel::ConversationIdRole) != INVALID_THREAD_ID;
    } else {
        if (sourceModel()->data(index, Qt::DisplayRole).toString().contains(filterRegExp())) {
           return true;
        }

        // This block of code compares each address in the multi target conversation to find a match
        QList<ConversationAddress> addressList = sourceModel()->data(index, ConversationListModel::AddressesRole).value<QList<ConversationAddress>>();
        for (const ConversationAddress address : addressList) {
            if (address.address().contains(filterRegExp())) {
                return true;
            }
        }
    }
    return false;
}

bool ConversationsSortFilterProxyModel::doesAddressExists(const QString &address)
{
    for(int i = 0; i < rowCount(); ++i) {
        if (!data(index(i, 0), ConversationListModel::MultitargetRole).toBool()) {
            QVariant senderAddress = data(index(i, 0), ConversationListModel::SenderRole);
            if (SmsHelper::isPhoneNumberMatch(senderAddress.toString(), address)) {
                return true;
            }
        }
    }
    return false;
}
