/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2020 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "deviceindicator.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <KLocalizedString>

#include "interfaces/dbusinterfaces.h"

#include <dbushelper.h>
#include <dbushelpers.h>
#include <systray_actions.h>

DeviceIndicator::DeviceIndicator(DeviceDbusInterface* device)
    : QMenu(device->name(), nullptr)
    , m_device(device)
    , m_remoteCommandsInterface(new RemoteCommandsDbusInterface(m_device->id()))
{
    setIcon(QIcon::fromTheme(device->iconName()));

    connect(device, SIGNAL(nameChanged(QString)), this, SLOT(setText(QString)));

    // Battery status
    auto battery = new BatteryAction(device);
    addAction(battery);
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_battery")),
                    [battery](bool available) {
                        battery->setVisible(available); 
                        battery->setDisabled(available); 
                    }, this);

    auto connectivity = new ConnectivityAction(device);
    addAction(connectivity);
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_connectivity_report")),
                    [connectivity](bool available) {
                        connectivity->setVisible(available); 
                        connectivity->setDisabled(available); 
                    }, this);

    this->addSeparator();

    // Browse device filesystem
    auto browse = addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Browse device"));
    connect(browse, &QAction::triggered, device, [device](){
        SftpDbusInterface* sftpIface = new SftpDbusInterface(device->id(), device);
        sftpIface->startBrowsing();
        sftpIface->deleteLater();
    });
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_sftp")), [browse](bool available) { browse->setVisible(available); }, this);

    // Find device
    auto findDevice = addAction(QIcon::fromTheme(QStringLiteral("irc-voice")), i18n("Ring device"));
    connect(findDevice, &QAction::triggered, device, [device](){
        FindMyPhoneDeviceDbusInterface* iface = new FindMyPhoneDeviceDbusInterface(device->id(), device);
        iface->ring();
        iface->deleteLater();
    });
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_findmyphone")), [findDevice](bool available) { findDevice->setVisible(available); }, this);

    // Get a photo
    auto getPhoto = addAction(QIcon::fromTheme(QStringLiteral("camera-photo")), i18n("Get a photo"));
    connect(getPhoto, &QAction::triggered, this, [device](){
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), QStringLiteral("/modules/kdeconnect/devices/") + device->id() + QStringLiteral("/photo"), QStringLiteral("org.kde.kdeconnect.device.photo"), QStringLiteral("requestPhoto"));
        msg.setArguments({QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first() + QDateTime::currentDateTime().toString(QStringLiteral("/dd-MM-yy_hh-mm-ss.png"))});
        blockOnReply(DBusHelper::sessionBus().asyncCall(msg));
    });
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_photo")), [getPhoto](bool available) { getPhoto->setVisible(available); }, this);

    // Send file
    const QString kdeconnectHandlerExecutable = QStandardPaths::findExecutable(QStringLiteral("kdeconnect-handler"));
    if (!kdeconnectHandlerExecutable.isEmpty()) {
        auto handlerApp = addAction(QIcon::fromTheme(QStringLiteral("document-share")), i18n("Send a file/URL"));
        QObject::connect(handlerApp, &QAction::triggered, device, [device, kdeconnectHandlerExecutable] () {
            QProcess::startDetached(kdeconnectHandlerExecutable, { QStringLiteral("--device"), device->id() });
        });
        handlerApp->setVisible(true);
    }

    // SMS Messages
    const QString kdeconnectsmsExecutable = QStandardPaths::findExecutable(QStringLiteral("kdeconnect-sms"), { QCoreApplication::applicationDirPath() });
    if (!kdeconnectsmsExecutable.isEmpty()) {
        auto smsapp = addAction(QIcon::fromTheme(QStringLiteral("message-new")), i18n("SMS Messages..."));
        QObject::connect(smsapp, &QAction::triggered, device, [device, kdeconnectsmsExecutable] () {
            QProcess::startDetached(kdeconnectsmsExecutable, { QStringLiteral("--device"), device->id() });
        });
        setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_sms")), [smsapp](bool available) { smsapp->setVisible(available); }, this);
    }

    // Run command
    QMenu* remoteCommandsMenu = new QMenu(i18n("Run command"), this);
    QAction* menuAction = remoteCommandsMenu->menuAction();
    QAction* addCommandAction = remoteCommandsMenu->addAction(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add commands"));
    connect(addCommandAction, &QAction::triggered, m_remoteCommandsInterface, &RemoteCommandsDbusInterface::editCommands);

    addAction(menuAction);
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_remotecommands")), [this, remoteCommandsMenu, menuAction](bool available) {
        menuAction->setVisible(available);

        if (!available)
            return;

        const auto cmds = QJsonDocument::fromJson(m_remoteCommandsInterface->commands()).object();

        for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it!=itEnd; ++it) {
            const QJsonObject cont = it->toObject();
            QString key = it.key();
            QAction* action = remoteCommandsMenu->addAction(cont.value(QStringLiteral("name")).toString());
            connect(action, &QAction::triggered, [this, key] {
                m_remoteCommandsInterface->triggerCommand(key);
            });
        }
    }, this);
}
