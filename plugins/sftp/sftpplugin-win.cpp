/**
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "sftpplugin-win.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>

#include <KLocalizedString>
#include <KPluginFactory>

#include "daemon.h"
#include "plugin_sftp_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SftpPlugin, "kdeconnect_sftp.json")

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , deviceId(device()->id())
{
}

bool SftpPlugin::startBrowsing()
{
    NetworkPacket np(PACKET_TYPE_SFTP_REQUEST, {{QStringLiteral("startBrowsing"), true}});
    sendPacket(np);
    return false;
}

void SftpPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("errorMessage"))) {
        QString message = np.get<QString>(QStringLiteral("errorMessage"));
        qCWarning(KDECONNECT_PLUGIN_SFTP) << message;
        Daemon::instance()->reportError(device()->name(), message);
        return;
    }

    static const QSet<QString> expectedFields{QStringLiteral("user"), QStringLiteral("port"), QStringLiteral("path"), QStringLiteral("password")};
    const QStringList receivedFieldsList = np.body().keys();
    const QSet<QString> receivedFields(receivedFieldsList.begin(), receivedFieldsList.end());
    if (!(expectedFields - receivedFields).isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid packet received.";
        return;
    }

    QString path;
    if (np.has(QStringLiteral("multiPaths"))) {
        QStringList paths = np.get<QStringList>(QStringLiteral("multiPaths"));
        if (paths.size() == 1) {
            path = paths[0];
        } else {
            path = QStringLiteral("/");
        }
    } else {
        path = np.get<QString>(QStringLiteral("path"));
    }
    if (!path.endsWith(QChar::fromLatin1('/'))) {
        path += QChar::fromLatin1('/');
    }

    QString url_string = QStringLiteral("sftp://%1:%2@%3:%4%5")
                             .arg(np.get<QString>(QStringLiteral("user")),
                                  np.get<QString>(QStringLiteral("password")),
                                  np.get<QString>(QStringLiteral("ip")),
                                  np.get<QString>(QStringLiteral("port")),
                                  path);
    static QRegularExpression uriRegex(QStringLiteral("^sftp://kdeconnect:\\w+@\\d+.\\d+.\\d+.\\d+:17[3-6][0-9]/$"));
    if (!uriRegex.match(url_string).hasMatch()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid URL invoked. If the problem persists, contact the developers.";
    }

    if (!QDesktopServices::openUrl(QUrl(url_string))) {
        QMessageBox::critical(nullptr,
                              i18n("KDE Connect"),
                              i18n("Cannot handle SFTP protocol. Apologies for the inconvenience"),
                              QMessageBox::Abort,
                              QMessageBox::Abort);
    }
}

#include "moc_sftpplugin-win.cpp"
#include "sftpplugin-win.moc"
