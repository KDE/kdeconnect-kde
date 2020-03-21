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

#ifndef CONVERSATIONLISTMODEL_H
#define CONVERSATIONLISTMODEL_H

#include <QStandardItemModel>
#include <QLoggingCategory>
#include <QQmlParserStatus>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL)


class OurSortFilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
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

    Q_INVOKABLE void setOurFilterRole(int role);

    OurSortFilterProxyModel();
    ~OurSortFilterProxyModel();

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

class ConversationListModel
    : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    ConversationListModel(QObject* parent = nullptr);
    ~ConversationListModel();

    enum Roles {
        /* Roles which apply while working as a single message */
        FromMeRole = Qt::UserRole,
        SenderRole,      // The sender of the message. Undefined if this is an outgoing message
        DateRole,        // The date of this message
        /* Roles which apply while working as the head of a conversation */
        AddressesRole,      // The Addresses involved in the conversation
        ConversationIdRole, // The ThreadID of the conversation
        MultitargetRole,    // Indicate that this conversation is multitarget
    };
    Q_ENUM(Roles)

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &/*deviceId*/);

    Q_SCRIPTABLE void refresh();

public Q_SLOTS:
    void handleCreatedConversation(const QDBusVariant& msg);
    void handleConversationUpdated(const QDBusVariant& msg);
    void createRowFromMessage(const ConversationMessage& message);
    void printDBusError(const QDBusError& error);
    void displayContacts();

Q_SIGNALS:
    void deviceIdChanged();

private:
    /**
     * Get all conversations currently known by the conversationsInterface, if any
     */
    void prepareConversationsList();

    QStandardItem* conversationForThreadId(qint32 threadId);
    QStandardItem* getConversationForAddress(const QString& address);

    DeviceConversationsDbusInterface* m_conversationsInterface;
    QString m_deviceId;
};

#endif // CONVERSATIONLISTMODEL_H
