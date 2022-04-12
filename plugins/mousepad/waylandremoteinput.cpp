/**
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "waylandremoteinput.h"

#include <QSizeF>
#include <QDebug>

#include <KLocalizedString>

#include <QtWaylandClient/qwaylandclientextension.h>
#include "qwayland-fake-input.h"

#include <linux/input.h>

class FakeInput : public QWaylandClientExtensionTemplate<FakeInput>, public QtWayland::org_kde_kwin_fake_input
{
public:
    FakeInput()
        : QWaylandClientExtensionTemplate<FakeInput>(4)
    {
    }
};


WaylandRemoteInput::WaylandRemoteInput(QObject* parent)
    : AbstractRemoteInput(parent)
    , m_waylandAuthenticationRequested(false)
{
    m_fakeInput = new FakeInput;
}

WaylandRemoteInput::~WaylandRemoteInput()
{
    delete m_fakeInput;
}

bool WaylandRemoteInput::handlePacket(const NetworkPacket& np)
{
    if (!m_fakeInput->isActive()) {
        return true;
    }

    if (!m_waylandAuthenticationRequested) {
        m_fakeInput->authenticate(i18n("KDE Connect"), i18n("Use your phone as a touchpad and keyboard"));
        m_waylandAuthenticationRequested = true;
    }

    const float dx = np.get<float>(QStringLiteral("dx"), 0);
    const float dy = np.get<float>(QStringLiteral("dy"), 0);

    const bool isSingleClick = np.get<bool>(QStringLiteral("singleclick"), false);
    const bool isDoubleClick = np.get<bool>(QStringLiteral("doubleclick"), false);
    const bool isMiddleClick = np.get<bool>(QStringLiteral("middleclick"), false);
    const bool isRightClick = np.get<bool>(QStringLiteral("rightclick"), false);
    const bool isSingleHold = np.get<bool>(QStringLiteral("singlehold"), false);
    const bool isSingleRelease = np.get<bool>(QStringLiteral("singlerelease"), false);
    const bool isScroll = np.get<bool>(QStringLiteral("scroll"), false);
    const QString key = np.get<QString>(QStringLiteral("key"), QLatin1String(""));
    const int specialKey = np.get<int>(QStringLiteral("specialKey"), 0);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isSingleHold || isSingleRelease || isScroll || !key.isEmpty() || specialKey) {

        if (isSingleClick) {
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);

        } else if (isDoubleClick) {
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);

            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isMiddleClick) {
            m_fakeInput->button(BTN_MIDDLE, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_MIDDLE, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isRightClick) {
            m_fakeInput->button(BTN_RIGHT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_RIGHT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isSingleHold){
            //For drag'n drop
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
        } else if (isSingleRelease){
            //For drag'n drop. NEVER USED (release is done by tapping, which actually triggers a isSingleClick). Kept here for future-proofness.
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isScroll) {
            m_fakeInput->axis(WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(-dy));

        } else if (!key.isEmpty() || specialKey) {
            // TODO: implement key support
        }

    } else { //Is a mouse move event
        m_fakeInput->pointer_motion(wl_fixed_from_double(dx), wl_fixed_from_double(dy));
    }
    return true;
}
