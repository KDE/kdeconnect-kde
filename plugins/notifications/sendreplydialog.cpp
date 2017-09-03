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

#include "sendreplydialog.h"

#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QBoxLayout>

#include <KLocalizedString>

#include "ui_sendreplydialog.h"

SendReplyDialog::SendReplyDialog(const QString& originalMessage, const QString& replyId, const QString& topicName, QWidget* parent)
    : QDialog(parent)
    , m_replyId(replyId)
    , m_ui(new Ui::SendReplyDialog)
{
    m_ui->setupUi(this);
    m_ui->textView->setText(topicName + ": \n" + originalMessage);

    auto button = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    button->setText(i18n("Send"));

    connect(this, &QDialog::accepted, this, &SendReplyDialog::sendButtonClicked);
    setWindowTitle(topicName);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    setAttribute(Qt::WA_DeleteOnClose);
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
