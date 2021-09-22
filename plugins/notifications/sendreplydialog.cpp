/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendreplydialog.h"

#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QBoxLayout>
#include <QStandardPaths>
#include <QKeyEvent>

#include <KLocalizedString>

#include "ui_sendreplydialog.h"

SendReplyDialog::SendReplyDialog(const QString& originalMessage, const QString& replyId, const QString& topicName, QWidget* parent)
    : QDialog(parent)
    , m_replyId(replyId)
    , m_ui(new Ui::SendReplyDialog)
{
    m_ui->setupUi(this);
    m_ui->textView->setText(topicName + QStringLiteral(": \n") + originalMessage);

    auto button = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setText(i18n("Send"));

    auto textEdit = m_ui->replyEdit;
    connect(textEdit, &SendReplyTextEdit::send, this, &SendReplyDialog::sendButtonClicked);

    connect(this, &QDialog::accepted, this, &SendReplyDialog::sendButtonClicked);
    setWindowTitle(topicName);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    setAttribute(Qt::WA_DeleteOnClose);
    m_ui->replyEdit->setFocus();
}

SendReplyDialog::~SendReplyDialog() = default;

void SendReplyDialog::sendButtonClicked()
{
    Q_EMIT sendReply(m_replyId, m_ui->replyEdit->toPlainText());
    close();
}

QSize SendReplyDialog::sizeHint() const
{
    return QSize(512, 64);
}

SendReplyTextEdit::SendReplyTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

void SendReplyTextEdit::keyPressEvent(QKeyEvent* event)
{
    // Send reply on enter, except when shift + enter is pressed, then insert newline
    const int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if ((key == Qt::Key_Enter && (event->modifiers() == Qt::KeypadModifier)) || !event->modifiers()) {
            Q_EMIT send();
            event->accept();
            return;
        }
    }
    QTextEdit::keyPressEvent(event);
}
