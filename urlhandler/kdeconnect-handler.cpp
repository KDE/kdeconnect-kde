/*
 * Copyright 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include <QApplication>
#include <QDialog>
#include <QCommandLineParser>
#include <QComboBox>
#include <QTextStream>
#include <QUrl>
#include <QDBusMessage>
#include <QDBusConnection>

#include <KAboutData>
#include <KLocalizedString>

#include <interfaces/devicesmodel.h>
#include <interfaces/devicessortproxymodel.h>
#include <interfaces/dbusinterfaces.h>
#include <interfaces/dbushelpers.h>
#include "kdeconnect-version.h"
#include "ui_dialog.h"

/**
 * Show only devices that can be shared to
 */
class ShareDevicesProxyModel : public DevicesSortProxyModel
{
public:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override {
        const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        auto device = qobject_cast<DeviceDbusInterface*>(idx.data(DevicesModel::DeviceRole).value<QObject*>());
        return device->supportedPlugins().contains(QStringLiteral("kdeconnect_share"));
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QString description = i18n("KDE Connect URL handler");
    KAboutData about(QStringLiteral("kdeconnect-urlhandler"),
                     description,
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     description,
                     KAboutLicense::GPL, 
                     i18n("(C) 2017 Aleix Pol Gonzalez"));
    about.addAuthor( QStringLiteral("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@kde.org") );
    KAboutData::setApplicationData(about);

    QUrl urlToShare;
    {
        QCommandLineParser parser;
        parser.addPositionalArgument(QStringLiteral("url"), i18n("URL to share"));
        parser.addHelpOption();
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
        if (parser.positionalArguments().count() != 1) {
            parser.showHelp(1);
            return 1;
        }

        urlToShare = QUrl::fromUserInput(parser.positionalArguments().constFirst());
    }

    DevicesModel model;
    model.setDisplayFilter(DevicesModel::Paired | DevicesModel::Reachable);
    ShareDevicesProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    QDialog dialog;
    dialog.setWindowTitle(urlToShare.toDisplayString());
    Ui::Dialog uidialog;
    uidialog.setupUi(&dialog);
    uidialog.devicePicker->setModel(&proxyModel);

    if (urlToShare.scheme() == QLatin1String("tel")) {
        uidialog.label->setText(i18n("Device to call this phone number with:"));
        uidialog.urlLabel->setText(urlToShare.toDisplayString(QUrl::RemoveScheme));
    } else {
        uidialog.urlLabel->setText(urlToShare.toDisplayString());
    }

    if (dialog.exec() == QDialog::Accepted) {
        QUrl url = urlToShare;
        const int currentDeviceIndex = uidialog.devicePicker->currentIndex();
        if(!url.isEmpty() && currentDeviceIndex >= 0) {
            const QString device = proxyModel.index(currentDeviceIndex, 0).data(DevicesModel::IdModelRole).toString();

            QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), "/modules/kdeconnect/devices/"+device+"/share", QStringLiteral("org.kde.kdeconnect.device.share"), QStringLiteral("shareUrl"));
            msg.setArguments({ url.toString() });
            blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
            return 0;
        } else {
            QTextStream(stderr) << (i18n("Couldn't share %1", url.toString())) << endl;
            return 1;
        }
    } else {
        return 1;
    }
}
