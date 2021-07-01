/**
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "waylandremoteinput.h"

#include <QSizeF>
#include <QDebug>

#include <KLocalizedString>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/fakeinput.h>
#include <KWayland/Client/registry.h>

WaylandRemoteInput::WaylandRemoteInput(QObject* parent)
    : AbstractRemoteInput(parent)
    , m_waylandInput(nullptr)
    , m_waylandAuthenticationRequested(false)
{
    using namespace KWayland::Client;
    ConnectionThread* connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        qDebug() << "failed to get the Connection from Qt, Wayland remote input will not work";
        return;
    }
    Registry* registry = new Registry(this);
    registry->create(connection);
    connect(registry, &Registry::fakeInputAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandInput = registry->createFakeInput(name, version, this);
        }
    );
    connect(registry, &Registry::fakeInputRemoved, m_waylandInput, &QObject::deleteLater);
    registry->setup();
}

bool WaylandRemoteInput::handlePacket(const NetworkPacket& np)
{
    if (!m_waylandInput) {
        return false;
    }

    if (!m_waylandAuthenticationRequested) {
        m_waylandInput->authenticate(i18n("KDE Connect"), i18n("Use your phone as a touchpad and keyboard"));
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
            m_waylandInput->requestPointerButtonClick(Qt::LeftButton);
        } else if (isDoubleClick) {
            m_waylandInput->requestPointerButtonClick(Qt::LeftButton);
            m_waylandInput->requestPointerButtonClick(Qt::LeftButton);
        } else if (isMiddleClick) {
            m_waylandInput->requestPointerButtonClick(Qt::MiddleButton);
        } else if (isRightClick) {
            m_waylandInput->requestPointerButtonClick(Qt::RightButton);
        } else if (isSingleHold){
            //For drag'n drop
            m_waylandInput->requestPointerButtonPress(Qt::LeftButton);
        } else if (isSingleRelease){
            //For drag'n drop. NEVER USED (release is done by tapping, which actually triggers a isSingleClick). Kept here for future-proofness.
            m_waylandInput->requestPointerButtonRelease(Qt::LeftButton);
        } else if (isScroll) {
            m_waylandInput->requestPointerAxis(Qt::Vertical, -dy);
        } else if (!key.isEmpty() || specialKey) {
            // TODO: implement key support
        }

    } else { //Is a mouse move event
        m_waylandInput->requestPointerMove(QSizeF(dx, dy));
    }
    return true;
}
