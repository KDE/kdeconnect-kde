/**
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "sftpplugin-win.h"

#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QProcess>

#include <KLocalizedString>
#include <KPluginFactory>

#include "kdeconnectconfig.h"
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
    QStringList receivedFieldsList = np.body().keys();
    QSet<QString> receivedFields(receivedFieldsList.begin(), receivedFieldsList.end());
    if (!(expectedFields - receivedFields).isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid packet received.";
        for (QString missingField : (expectedFields - receivedFields)) {
            qCWarning(KDECONNECT_PLUGIN_SFTP) << "Field" << missingField << "missing from packet.";
        }
        return;
    }
    if (np.has(QStringLiteral("errorMessage"))) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << np.get<QString>(QStringLiteral("errorMessage"));
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

    QString url_string = QStringLiteral("sftp://%1;x-publickeyfile=%2@%3:%4%5")
                             .arg(np.get<QString>(QStringLiteral("user")),
                                  QLatin1StringView(QUrl::toPercentEncoding(KdeConnectConfig::instance().privateKeyPath())),
                                  np.get<QString>(QStringLiteral("ip")),
                                  np.get<QString>(QStringLiteral("port")),
                                  path);

    if (!QDesktopServices::openUrl(QUrl(url_string))) {
        QMessageBox::critical(nullptr,
                              i18n("KDE Connect"),
                              i18n("Couldn't find a program to handle sftp:// URLs, please install WinSCP or another SFTP client."),
                              QMessageBox::Abort,
                              QMessageBox::Abort);
    }
}

#include "moc_sftpplugin-win.cpp"
#include "sftpplugin-win.moc"
