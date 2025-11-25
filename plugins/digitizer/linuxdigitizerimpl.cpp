/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "linuxdigitizerimpl.h"

#include <core/daemon.h>

#include "abstractdigitizerimpl.h"
#include "plugin_digitizer_debug.h"
#include "toolevent.h"

#include <KLocalizedString>
#include <KMessageBox>

LinuxDigitizerImpl::LinuxDigitizerImpl(QObject *parent, const Device *device)
    : AbstractDigitizerImpl(parent, device)
    , m_dev(nullptr)
    , m_uinput(nullptr)
{
}

static int enableAbsCode(libevdev *dev, unsigned int code, int min, int max, int resolution)
{
    input_absinfo absinfo = {
        .value = 0,
        .minimum = min,
        .maximum = max,
        .fuzz = 0,
        .flat = 0,
        .resolution = resolution,
    };

    return libevdev_enable_event_code(dev, EV_ABS, code, &absinfo);
}

void LinuxDigitizerImpl::startSession(int width, int height, int resolutionX, int resolutionY)
{
    // If a session already exists, we must first end the session, then create a new one.
    if (m_dev != nullptr) {
        endSession();
    }

    qCInfo(KDECONNECT_PLUGIN_DIGITIZER) << "Starting a new drawing tablet session."
                                        << "width:" << width << "height:" << height << "resolutionX:" << resolutionX << "resolutionY:" << resolutionY;

    m_dev = libevdev_new();
    libevdev_set_name(m_dev, device()->name().toUtf8().constData());
    libevdev_set_id_bustype(m_dev, BUS_VIRTUAL);

    libevdev_enable_event_type(m_dev, EV_SYN);

    libevdev_enable_property(m_dev, INPUT_PROP_DIRECT);
    libevdev_enable_property(m_dev, INPUT_PROP_POINTER);

    libevdev_enable_event_type(m_dev, EV_KEY);
    libevdev_enable_event_code(m_dev, EV_KEY, BTN_TOOL_PEN, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, BTN_TOOL_RUBBER, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, BTN_TOUCH, nullptr);

    libevdev_enable_event_type(m_dev, EV_ABS);
    enableAbsCode(m_dev, ABS_X, 0, width - 1, resolutionX);
    enableAbsCode(m_dev, ABS_Y, 0, height - 1, resolutionY);
    enableAbsCode(m_dev, ABS_PRESSURE, 0, 1023, 0);

    if (int err = libevdev_uinput_create_from_device(m_dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_uinput) != 0) {
        qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to create uinput device:" << strerror(-err);

        Daemon::instance()->sendSimpleNotification(QStringLiteral("digitizerError"),
                                                   i18n("KDE Connect Error"),
                                                   i18n("Unable to create a virtual drawing tablet. If this error persists, file a bug report."),
                                                   QStringLiteral("error"));
        return;
    }

    qCInfo(KDECONNECT_PLUGIN_DIGITIZER) << "Started drawing tablet session successfully.";
}

void LinuxDigitizerImpl::endSession()
{
    qCInfo(KDECONNECT_PLUGIN_DIGITIZER) << "Ending drawing tablet session.";

    if (m_uinput != nullptr) {
        libevdev_uinput_destroy(m_uinput);
        m_uinput = nullptr;
    }

    if (m_dev != nullptr) {
        libevdev_free(m_dev);
        m_dev = nullptr;
    }
}

void LinuxDigitizerImpl::onToolEvent(ToolEvent event)
{
    if (m_uinput == nullptr) {
        return;
    }

    bool anyEvent = false;
    int err = 0;

    if (event.active && !event.active.value()) {
        // If the device reports an `active = false`, we set everything to zero (including the BTN_TOOL_* codes) to indicate
        // to applications that the device is not tracking the tool anymore.

        err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOOL_PEN, 0);
        err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOOL_RUBBER, 0);
        err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOUCH, 0);
        err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_PRESSURE, 0);
        err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_X, 0);
        err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_Y, 0);

        anyEvent = true;
    } else {
        bool dontReportPressure = false;

        if (event.touching) {
            // We must jump through some hoops in order to set the pressure to zero if the tool is not touching the screen,
            // otherwise apps will think otherwise.

            if (event.touching.value()) {
                err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOUCH, 1);
            } else {
                err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOUCH, 0);
                err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_PRESSURE, 0);

                dontReportPressure = true;
            }

            anyEvent = true;
        }

        if (event.tool) {
            QString tool = event.tool.value();
            err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOOL_PEN, tool == TOOL_PEN);
            err = libevdev_uinput_write_event(m_uinput, EV_KEY, BTN_TOOL_RUBBER, tool == TOOL_RUBBER);
            anyEvent = true;
        }

        if (event.x) {
            err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_X, event.x.value());
            anyEvent = true;
        }

        if (event.y) {
            err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_Y, event.y.value());
            anyEvent = true;
        }

        if (!dontReportPressure && event.pressure) {
            err = libevdev_uinput_write_event(m_uinput, EV_ABS, ABS_PRESSURE, event.pressure.value() * 1023);
            anyEvent = true;
        }
    }

    if (anyEvent) {
        err = libevdev_uinput_write_event(m_uinput, EV_SYN, SYN_REPORT, 0);
    }

    if (err != 0) {
        qCCritical(KDECONNECT_PLUGIN_DIGITIZER) << "Failed to emit events:" << strerror(-err);

        Daemon::instance()->sendSimpleNotification(
            QStringLiteral("digitizerError"),
            i18n("KDE Connect Error"),
            i18n("Unable to process one or more input events for the virtual device. If this error persists, file a bug report."),
            QStringLiteral("error"));

        endSession();
    }
}

#include "moc_linuxdigitizerimpl.cpp"
