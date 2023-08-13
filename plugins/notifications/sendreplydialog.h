/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDialog>
#include <QSize>
#include <QTextEdit>

namespace Ui
{
class SendReplyDialog;
}

class SendReplyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendReplyDialog(const QString &originalMessage, const QString &replyId, const QString &topicName, QWidget *parent = nullptr);
    ~SendReplyDialog() override;

    QSize sizeHint() const override;

Q_SIGNALS:
    void sendReply(const QString &replyId, const QString &messageBody);

private:
    const QString m_replyId;
    const std::unique_ptr<Ui::SendReplyDialog> m_ui;
};

class SendReplyTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    SendReplyTextEdit(QWidget *parent);

    void keyPressEvent(QKeyEvent *event) override;

    Q_SIGNAL void send();
};
