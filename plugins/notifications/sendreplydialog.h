/**
 * Copyright 2015 Albert Vaca <albertvaka@gmail.com>
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

#ifndef SENDREPLYDIALOG_H
#define SENDREPLYDIALOG_H

#include <QDialog>
#include <QSize>

namespace Ui { class SendReplyDialog; }

class SendReplyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendReplyDialog(const QString& originalMessage, const QString& replyId, const QString& topicName, QWidget* parent = nullptr);
    ~SendReplyDialog() override;

    QSize sizeHint() const override;

private Q_SLOTS:
    void sendButtonClicked();

Q_SIGNALS:
    void sendReply(const QString& replyId, const QString& messageBody);

private:
    const QString m_replyId;
    const QScopedPointer<Ui::SendReplyDialog> m_ui;
};

#endif
