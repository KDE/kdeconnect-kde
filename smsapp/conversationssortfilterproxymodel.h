/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CONVERSATIONSSORTFILTERPROXYMODEL_H
#define CONVERSATIONSSORTFILTERPROXYMODEL_H

#include <QLoggingCategory>
#include <QQmlParserStatus>
#include <QSortFilterProxyModel>

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_SORT_FILTER_PROXY_MODEL)

class ConversationsSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder)
public:
    Qt::SortOrder sortOrder() const
    {
        return m_sortOrder;
    }
    void setSortOrder(Qt::SortOrder sortOrder)
    {
        if (m_sortOrder != sortOrder) {
            m_sortOrder = sortOrder;
            sortNow();
        }
    }
    void classBegin() override
    {
    }
    void componentComplete() override
    {
        m_completed = true;
        sortNow();
    }

    Q_INVOKABLE void setConversationsFilterRole(int role);

    /**
     * This method gets name of conversations or contact if it find any matching address
     * Needed to check if the conversation or contact already exist or no before adding an arbitrary contact
     */
    Q_INVOKABLE bool doesAddressExists(const QString &address);

    ConversationsSortFilterProxyModel();
    ~ConversationsSortFilterProxyModel() override;

protected:
    bool lessThan(const QModelIndex &leftIndex, const QModelIndex &rightIndex) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    void sortNow()
    {
        if (m_completed && dynamicSortFilter())
            sort(0, m_sortOrder);
    }

    bool m_completed = false;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

#endif // CONVERSATIONSSORTFILTERPROXYMODEL_H
