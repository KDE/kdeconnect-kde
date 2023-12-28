/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationslistener.h"

#include <QBuffer>
#include <QImage>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>

#include "notifyingapplication.h"
#include "plugin_sendnotifications_debug.h"
#include <core/kdeconnectplugin.h>
#include <core/kdeconnectpluginconfig.h>

NotificationsListener::NotificationsListener(KdeConnectPlugin *aPlugin)
    : QObject(aPlugin)
    , m_plugin(aPlugin)
{
    setTranslatedAppName();
    loadApplications();

    connect(m_plugin->config(), &KdeConnectPluginConfig::configChanged, this, &NotificationsListener::loadApplications);
}

NotificationsListener::~NotificationsListener()
{
    qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Destroying NotificationsListener";
}

void NotificationsListener::setTranslatedAppName()
{
    QString filePath =
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications5/kdeconnect.notifyrc"), QStandardPaths::LocateFile);
    if (filePath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS)
            << "Couldn't find kdeconnect.notifyrc to hide kdeconnect notifications on the devices. Using default name.";
        m_translatedAppName = QStringLiteral("KDE Connect");
        return;
    }

    KConfig config(filePath, KConfig::OpenFlag::SimpleConfig);
    KConfigGroup globalgroup(&config, QStringLiteral("Global"));
    m_translatedAppName = globalgroup.readEntry(QStringLiteral("Name"), QStringLiteral("KDE Connect"));
}

void NotificationsListener::loadApplications()
{
    m_applications.clear();
    const QVariantList list = m_plugin->config()->getList(QStringLiteral("applications"));
    for (const auto &a : list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!m_applications.contains(app.name))
            m_applications.insert(app.name, app);
    }
    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Loaded" << m_applications.size() << " applications";
}

bool NotificationsListener::checkApplicationName(const QString &appName, std::optional<std::reference_wrapper<const QString>> iconName)
{
    if (m_translatedAppName == appName) {
        return false;
    }

    auto it = m_applications.constFind(appName);
    if (it == m_applications.cend()) {
        // new application -> add to config
        NotifyingApplication app;
        app.name = appName;
        if (iconName.has_value()) {
            app.icon = iconName.value();
        }
        app.active = true;
        m_applications.insert(app.name, app);
        // update config:
        QVariantList list;
        for (const auto &a : std::as_const(m_applications)) {
            list << QVariant::fromValue<NotifyingApplication>(a);
        }
        m_plugin->config()->setList(QStringLiteral("applications"), list);

        return true;
    } else {
        return it->active;
    }
}

bool NotificationsListener::checkIsInBlacklist(const QString &appName, const QString &content)
{
    auto appIt = m_applications.constFind(appName);
    return appIt->blacklistExpression.isValid() && !appIt->blacklistExpression.pattern().isEmpty() && appIt->blacklistExpression.match(content).hasMatch();
}

QSharedPointer<QIODevice> NotificationsListener::iconFromQImage(const QImage &image) const
{
    QSharedPointer<QBuffer> buffer = QSharedPointer<QBuffer>(new QBuffer);
    if (!buffer->open(QIODevice::WriteOnly) && !image.save(buffer.data(), "PNG")) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }

    return buffer;
}

#include "moc_notificationslistener.cpp"
