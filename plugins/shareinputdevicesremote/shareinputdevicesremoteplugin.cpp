/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "shareinputdevicesremoteplugin.h"

#include <core/daemon.h>
#include <core/device.h>

#include <KPluginFactory>

#include <QDBusConnection>
#include <QGuiApplication>
#include <QPointF>
#include <QScreen>

using namespace Qt::StringLiterals;

#define PACKET_TYPE_MOUSEPAD_REQUEST u"kdeconnect.mousepad.request"_s
#define PACKET_TYPE_SHAREINPUTDEVICES u"kdeconnect.shareinputdevices"_s
#define PACKET_TYPE_SHAREINPUTDEVICES_REQUEST u"kdeconnect.shareinputdevices.request"_s

K_PLUGIN_CLASS_WITH_JSON(ShareInputDevicesRemotePlugin, "kdeconnect_shareinputdevicesremote.json")

ShareInputDevicesRemotePlugin::ShareInputDevicesRemotePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
}

void ShareInputDevicesRemotePlugin::receivePacket(const NetworkPacket &np)
{
    if (np.type() == PACKET_TYPE_SHAREINPUTDEVICES_REQUEST) {
        if (np.has(u"startEdge"_s)) {
            const Qt::Edge startEdge = np.get<Qt::Edge>(u"startEdge"_s);
            const QPointF delta = {np.get<double>(u"deltax"_s), np.get<double>(u"deltay"_s)};
            auto screens = QGuiApplication::screens();
            auto mousePadPlugin = device()->plugin(u"kdeconnect_mousepad"_s);
            if (screens.empty() || !mousePadPlugin) {
                NetworkPacket packet(PACKET_TYPE_SHAREINPUTDEVICES, {{u"releaseDelta"_s, QPointF(0, 0)}});
                sendPacket(packet);
                return;
            }
            // This is opposite to the sorting in inputcaptureession because here the opposite edge is wanted.
            // For example if on the source side a left barrier was crossed, this is one is entered from the right.
            if (startEdge == Qt::LeftEdge) {
                std::stable_sort(screens.begin(), screens.end(), [](QScreen *lhs, QScreen *rhs) {
                    return lhs->geometry().x() + lhs->geometry().width() > rhs->geometry().x() + rhs->geometry().width();
                });
                m_enterEdge = Qt::RightEdge;
                m_currentPosition = screens.front()->geometry().topRight() + delta;
            } else if (startEdge == Qt::RightEdge) {
                std::stable_sort(screens.begin(), screens.end(), [](QScreen *lhs, QScreen *rhs) {
                    return lhs->geometry().x() < rhs->geometry().x();
                });
                m_enterEdge = Qt::LeftEdge;
                m_currentPosition = screens.front()->geometry().topLeft() + delta;
            } else if (startEdge == Qt::TopEdge) {
                std::stable_sort(screens.begin(), screens.end(), [](QScreen *lhs, QScreen *rhs) {
                    return lhs->geometry().y() + lhs->geometry().height() > rhs->geometry().y() + rhs->geometry().height();
                });
                m_enterEdge = Qt::BottomEdge;
                m_currentPosition = screens.front()->geometry().bottomLeft() + delta;
            } else {
                std::stable_sort(screens.begin(), screens.end(), [](QScreen *lhs, QScreen *rhs) {
                    return lhs->geometry().y() < rhs->geometry().y();
                });
                m_enterEdge = Qt::TopEdge;
                m_currentPosition = screens.front()->geometry().topLeft() + delta;
            }
            m_enterScreen = screens.front();
            // Send a packet from this local device to the mousepad plugin because the position could only
            // be calculated here
            NetworkPacket packet(PACKET_TYPE_MOUSEPAD_REQUEST, {{u"x"_s, m_currentPosition.x()}, {u"y"_s, m_currentPosition.y()}});
            mousePadPlugin->receivePacket(packet);
        }
    }

    if (np.type() == PACKET_TYPE_MOUSEPAD_REQUEST) {
        if (np.has(u"dx"_s) && m_enterScreen) {
            m_currentPosition += QPointF(np.get<double>(u"dx"_s), np.get<double>(u"dy"_s));
            auto sendRelease = [this](const QPointF &delta) {
                NetworkPacket packet(PACKET_TYPE_SHAREINPUTDEVICES, {{u"releaseDeltax"_s, delta.x()}, {u"releaseDeltay"_s, delta.y()}});
                sendPacket(packet);
            };
            if (m_enterEdge == Qt::LeftEdge && m_currentPosition.x() < m_enterScreen->geometry().x()) {
                const QPointF delta = m_currentPosition - m_enterScreen->geometry().topLeft();
                sendRelease(delta);
            } else if (m_enterEdge == Qt::RightEdge && m_currentPosition.x() > m_enterScreen->geometry().x() + m_enterScreen->geometry().width()) {
                const QPointF delta = m_currentPosition - m_enterScreen->geometry().topRight();
                sendRelease(delta);
            } else if (m_enterEdge == Qt::TopEdge && m_currentPosition.y() < m_enterScreen->geometry().y()) {
                const QPointF delta = m_currentPosition - m_enterScreen->geometry().topLeft();
                sendRelease(delta);
            } else if (m_enterEdge == Qt::BottomEdge && m_currentPosition.y() > m_enterScreen->geometry().y() + m_enterScreen->geometry().height()) {
                const QPointF delta = m_currentPosition - m_enterScreen->geometry().bottomLeft();
                sendRelease(delta);
            }
        }
    }
}

QString ShareInputDevicesRemotePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/shareinputdevicesremote").arg(device()->id());
}

#include "shareinputdevicesremoteplugin.moc"
