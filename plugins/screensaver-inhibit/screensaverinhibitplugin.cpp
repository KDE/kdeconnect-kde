/**
 * SPDX-FileCopyrightText: 2014 Pramod Dematagoda <pmdematagoda@mykolab.ch>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "screensaverinhibitplugin.h"

#include "generated/systeminterfaces/screensaver.h"
#include <KLocalizedString>
#include <KPluginFactory>
#include <QDBusConnection>

K_PLUGIN_CLASS_WITH_JSON(ScreensaverInhibitPlugin, "kdeconnect_screensaver_inhibit.json")

#define INHIBIT_SERVICE QStringLiteral("org.freedesktop.ScreenSaver")
#define INHIBIT_PATH QStringLiteral("/ScreenSaver")

ScreensaverInhibitPlugin::ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    OrgFreedesktopScreenSaverInterface inhibitInterface(INHIBIT_SERVICE, INHIBIT_PATH, QDBusConnection::sessionBus(), this);

    inhibitCookie = inhibitInterface.Inhibit(QStringLiteral("org.kde.kdeconnect.daemon"), i18n("Phone is connected"));
}

ScreensaverInhibitPlugin::~ScreensaverInhibitPlugin()
{
    if (inhibitCookie == 0)
        return;

    OrgFreedesktopScreenSaverInterface inhibitInterface(INHIBIT_SERVICE, INHIBIT_PATH, QDBusConnection::sessionBus(), this);
    inhibitInterface.UnInhibit(inhibitCookie);

    /*
     * Simulate user activity because what ever manages the screensaver does not seem to start the timer
     * automatically when all inhibitions are lifted and the user does nothing which results in an
     * unlocked desktop which would be dangerous. Ideally we should not be doing this and the screen should
     * be locked anyway.
     */
    inhibitInterface.SimulateUserActivity();
}

#include "moc_screensaverinhibitplugin.cpp"
#include "screensaverinhibitplugin.moc"
