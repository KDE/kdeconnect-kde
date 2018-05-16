/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "conversationmodel.h"

ConversationModel::ConversationModel(QObject* parent)
    : QStandardItemModel(parent)
{
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    setItemRoleNames(roles);
}

QString ConversationModel::threadId() const
{
    return {};
}

void ConversationModel::setThreadId(const QString &threadId)
{
    clear();
    appendRow(new QStandardItem(threadId + QStringLiteral(" - A")));
    appendRow(new QStandardItem(threadId + QStringLiteral(" - A1")));
    appendRow(new QStandardItem(threadId + QStringLiteral(" - A2")));
    appendRow(new QStandardItem(threadId + QStringLiteral(" - A3")));
}
