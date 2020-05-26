/**
 * Copyright 2019 Piyush Aggarwal<piyushaggarwal002@gmail.com>
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
#include "sftpplugin-win.h"

#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegularExpression>

#include <KLocalizedString>
#include <KPluginFactory>

#include "plugin_sftp_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SftpPlugin, "kdeconnect_sftp.json")

SftpPlugin::SftpPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    deviceId = device()->id();
}

SftpPlugin::~SftpPlugin(){}

bool SftpPlugin::startBrowsing()
{
    NetworkPacket np(PACKET_TYPE_SFTP_REQUEST, {{QStringLiteral("startBrowsing"), true}});
    sendPacket(np);
    return false;
}

bool SftpPlugin::receivePacket(const NetworkPacket& np)
{
    if (!(expectedFields - np.body().keys().toSet()).isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid packet received.";
        for (QString missingField: (expectedFields - np.body().keys().toSet()).toList()) {
            qCWarning(KDECONNECT_PLUGIN_SFTP) << "Field" << missingField << "missing from packet.";
        }
        return false;
    }
    if (np.has(QStringLiteral("errorMessage"))) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << np.get<QString>(QStringLiteral("errorMessage"));
        return false;
    }

    QString path;
    if (np.has(QStringLiteral("multiPaths"))) {
        path = QStringLiteral("/");
    } else {
        path = np.get<QString>(QStringLiteral("path"));
    }

    QString url_string = QStringLiteral("sftp://%1:%2@%3:%4%5").arg(
                            np.get<QString>(QStringLiteral("user")),
                            np.get<QString>(QStringLiteral("password")),
                            np.get<QString>(QStringLiteral("ip")),
                            np.get<QString>(QStringLiteral("port")),
                            path
                         );
    static QRegularExpression uriRegex(QStringLiteral("^sftp://kdeconnect:\\w+@\\d+.\\d+.\\d+.\\d+:17[3-6][0-9]/$"));
    if (!uriRegex.match(url_string).hasMatch()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid URL invoked. If the problem persists, contact the developers.";
    }

    if (!QDesktopServices::openUrl(QUrl(url_string))) {
        QMessageBox::critical(nullptr, i18n("KDE Connect"), i18n("Cannot handle SFTP protocol. Apologies for the inconvenience"),
        QMessageBox::Abort,
        QMessageBox::Abort);
    }
    return true;
}

#include "sftpplugin-win.moc"
