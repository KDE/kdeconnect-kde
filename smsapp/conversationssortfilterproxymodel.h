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

#ifndef CONVERSATIONSSORTFILTERPROXYMODEL_H
#define CONVERSATIONSSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QLoggingCategory>
#include <QQmlParserStatus>

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_SORT_FILTER_PROXY_MODEL)

class ConversationsSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder)
public:

    Qt::SortOrder sortOrder() const { return m_sortOrder; }
    void setSortOrder(Qt::SortOrder sortOrder) {
        if (m_sortOrder != sortOrder) {
            m_sortOrder = sortOrder;
            sortNow();
        }
    }
    void classBegin() override {}
    void componentComplete() override {
        m_completed = true;
        sortNow();
    }

    Q_INVOKABLE void setConversationsFilterRole(int role);

    /**
     * This method gets name of conversations or contact if it find any matching address
     * Needed to check if the conversation or contact already exist or no before adding an arbitrary contact
     */
    Q_INVOKABLE bool doesPhoneNumberExists(const QString& address);

    ConversationsSortFilterProxyModel();
    ~ConversationsSortFilterProxyModel();

protected:
    bool lessThan(const QModelIndex& leftIndex, const QModelIndex& rightIndex) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    void sortNow() {
        if (m_completed && dynamicSortFilter())
            sort(0, m_sortOrder);
    }

    bool m_completed = false;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};

#endif // CONVERSATIONSSORTFILTERPROXYMODEL_H
