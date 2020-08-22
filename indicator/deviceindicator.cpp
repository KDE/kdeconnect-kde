/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "deviceindicator.h"
#include <QFileDialog>
#include <KLocalizedString>

#include "interfaces/dbusinterfaces.h"

#include <dbushelper.h>

class BatteryAction : public QAction
{
Q_OBJECT
public:
    BatteryAction(DeviceDbusInterface* device)
        : QAction(nullptr)
        , m_batteryIface(device->id())
    {
        setCharge(m_batteryIface.charge());
        setCharging(m_batteryIface.isCharging());

        connect(&m_batteryIface, &BatteryDbusInterface::refreshedProxy, this, [this]{
            setCharge(m_batteryIface.charge());
            setCharging(m_batteryIface.isCharging());
        });

        setIcon(QIcon::fromTheme(QStringLiteral("battery")));

        update();
    }

    void update() {
        if (m_charge < 0)
            setText(i18n("No Battery"));
        else if (m_charging)
            setText(i18n("Battery: %1% (Charging)", m_charge));
        else
            setText(i18n("Battery: %1%", m_charge));
    }

private Q_SLOTS:
    void setCharge(int charge) { m_charge = charge; update(); }
    void setCharging(bool charging) { m_charging = charging; update(); }

private:
    BatteryDbusInterface m_batteryIface;
    int m_charge = -1;
    bool m_charging = false;
};


DeviceIndicator::DeviceIndicator(DeviceDbusInterface* device)
    : QMenu(device->name(), nullptr)
    , m_device(device)
    , m_remoteCommandsInterface(new RemoteCommandsDbusInterface(m_device->id()))
{
#ifdef Q_OS_WIN
    setIcon(QIcon(QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icons/hicolor/scalable/status/") + device->iconName() + QStringLiteral(".svg"))));
#else
    setIcon(QIcon::fromTheme(device->iconName()));
#endif

    connect(device, SIGNAL(nameChanged(QString)), this, SLOT(setText(QString)));

    auto battery = new BatteryAction(device);
    addAction(battery);
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_battery")),
                     [battery](bool available) { battery->setVisible(available);  }
                     , this);

    auto browse = addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Browse device"));
    connect(browse, &QAction::triggered, device, [device](){
        SftpDbusInterface* sftpIface = new SftpDbusInterface(device->id(), device);
        sftpIface->startBrowsing();
        sftpIface->deleteLater();
    });
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_sftp")), [browse](bool available) { browse->setVisible(available); }, this);

    auto findDevice = addAction(QIcon::fromTheme(QStringLiteral("irc-voice")), i18n("Ring device"));
    connect(findDevice, &QAction::triggered, device, [device](){
        FindMyPhoneDeviceDbusInterface* iface = new FindMyPhoneDeviceDbusInterface(device->id(), device);
        iface->ring();
        iface->deleteLater();
    });
    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_findmyphone")), [findDevice](bool available) { findDevice->setVisible(available); }, this);

    auto sendFile = addAction(QIcon::fromTheme(QStringLiteral("document-share")), i18n("Send file"));
    connect(sendFile, &QAction::triggered, device, [device, this](){
        const QUrl url = QFileDialog::getOpenFileUrl(parentWidget(), i18n("Select file to send to '%1'", device->name()), QUrl::fromLocalFile(QDir::homePath()));
        if (url.isEmpty())
            return;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), QStringLiteral("/modules/kdeconnect/devices/") + device->id() + QStringLiteral("/share"), QStringLiteral("org.kde.kdeconnect.device.share"), QStringLiteral("shareUrl"));
        msg.setArguments(QVariantList() << url.toString());
        DBusHelper::sessionBus().call(msg);
    });

    setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_share")), [sendFile](bool available) { sendFile->setVisible(available); }, this);

    // Search current application path
    const QString kdeconnectsmsExecutable = QStandardPaths::findExecutable(QStringLiteral("kdeconnect-sms"), { QCoreApplication::applicationDirPath() });
    if (!kdeconnectsmsExecutable.isEmpty()) {
        auto smsapp = addAction(QIcon::fromTheme(QStringLiteral("message-new")), i18n("SMS Messages..."));
        QObject::connect(smsapp, &QAction::triggered, device, [device, kdeconnectsmsExecutable] () {
            QProcess::startDetached(kdeconnectsmsExecutable, { QStringLiteral("--device"), device->id() });
        });
        setWhenAvailable(device->hasPlugin(QStringLiteral("kdeconnect_sms")), [smsapp](bool available) { smsapp->setVisible(available); }, this);
    }

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

#include "deviceindicator.moc"
