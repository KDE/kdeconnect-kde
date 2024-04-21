/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationssortfilterproxymodel.h"
#include "conversationlistmodel.h"
#include "smshelper.h"

#include <QLoggingCategory>
#include <QString>

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_SORT_FILTER_PROXY_MODEL, "kdeconnect.sms.conversations_sort_filter_proxy")

#define INVALID_THREAD_ID -1

ConversationsSortFilterProxyModel::ConversationsSortFilterProxyModel()
{
    setFilterRole(ConversationListModel::ConversationIdRole);
}

ConversationsSortFilterProxyModel::~ConversationsSortFilterProxyModel()
{
}

void ConversationsSortFilterProxyModel::setConversationsFilterRole(int role)
{
    setFilterRole(role);
}

bool ConversationsSortFilterProxyModel::lessThan(const QModelIndex &leftIndex, const QModelIndex &rightIndex) const
{
    qlonglong leftDataTimeStamp = sourceModel()->data(leftIndex, ConversationListModel::DateRole).toLongLong();
    qlonglong rightDataTimeStamp = sourceModel()->data(rightIndex, ConversationListModel::DateRole).toLongLong();

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
        if (sourceModel()->data(index, Qt::DisplayRole).toString().contains(filterRegularExpression())) {
            return true;
        }

        // This block of code compares each address in the multi target conversation to find a match
        const QList<ConversationAddress> addressList = sourceModel()->data(index, ConversationListModel::AddressesRole).value<QList<ConversationAddress>>();
        for (const ConversationAddress &address : addressList) {
            QString canonicalAddress = SmsHelper::canonicalizePhoneNumber(address.address());
            if (canonicalAddress.contains(filterRegularExpression())) {
                return true;
            }
        }
    }
    return false;
}

bool ConversationsSortFilterProxyModel::doesAddressExists(const QString &address)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (!data(index(i, 0), ConversationListModel::MultitargetRole).toBool()) {
            QVariant senderAddress = data(index(i, 0), ConversationListModel::SenderRole);
            if (SmsHelper::isPhoneNumberMatch(senderAddress.toString(), address)) {
                return true;
            }
        }
    }
    return false;
}

#include "moc_conversationssortfilterproxymodel.cpp"
