/**
 * SPDX-FileCopyrightText: 2014 Pramod Dematagoda <pmdematagoda@mykolab.ch>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "screensaverinhibitplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QDBusConnection>
#include <QDBusInterface>
#include "kdeconnect_screensaverinhibit_debug.h"

K_PLUGIN_CLASS_WITH_JSON(ScreensaverInhibitPlugin, "kdeconnect_screensaver_inhibit.json")

#define INHIBIT_SERVICE QStringLiteral("org.freedesktop.ScreenSaver")
#define INHIBIT_INTERFACE INHIBIT_SERVICE
#define INHIBIT_PATH QStringLiteral("/ScreenSaver")
#define INHIBIT_METHOD QStringLiteral("Inhibit")
#define UNINHIBIT_METHOD QStringLiteral("UnInhibit")
#define SIMULATE_ACTIVITY_METHOD QStringLiteral("SimulateUserActivity")

ScreensaverInhibitPlugin::ScreensaverInhibitPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    QDBusInterface inhibitInterface(INHIBIT_SERVICE, INHIBIT_PATH, INHIBIT_INTERFACE);

    QDBusMessage reply = inhibitInterface.call(INHIBIT_METHOD, QStringLiteral("org.kde.kdeconnect.daemon"), i18n("Phone is connected"));

    if (!reply.errorMessage().isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_SCREENSAVERINHIBIT) << "Unable to inhibit the screensaver: " << reply.errorMessage();
        inhibitCookie = 0;
    } else {
        // Store the cookie we receive, this will be sent back when sending the uninhibit call.
        inhibitCookie = reply.arguments().at(0).toUInt();
    }
}

ScreensaverInhibitPlugin::~ScreensaverInhibitPlugin()
{
    if (inhibitCookie == 0) return;

    QDBusInterface inhibitInterface(INHIBIT_SERVICE, INHIBIT_PATH, INHIBIT_INTERFACE);
    inhibitInterface.call(UNINHIBIT_METHOD, this->inhibitCookie);

    /*
     * Simulate user activity because what ever manages the screensaver does not seem to start the timer
     * automatically when all inhibitions are lifted and the user does nothing which results in an
     * unlocked desktop which would be dangerous. Ideally we should not be doing this and the screen should
     * be locked anyway.
     */
    inhibitInterface.call(SIMULATE_ACTIVITY_METHOD);
}

void ScreensaverInhibitPlugin::connected()
{
}

bool ScreensaverInhibitPlugin::receivePacket(const NetworkPacket& np)
{
    Q_UNUSED(np);
    return false;
}

#include "screensaverinhibitplugin.moc"
