/**
 * Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "presenterplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QQuickView>
#include <QQmlError>
#include <QQuickItem>
#include <QtGlobal>

#include <core/device.h>
#include <core/daemon.h>
#include <QScreen>
#include "plugin_presenter_debug.h"

K_PLUGIN_CLASS_WITH_JSON(PresenterPlugin, "kdeconnect_presenter.json")

class PresenterView : public QQuickView
{
public:
    PresenterView() {
        Qt::WindowFlags windowFlags = Qt::WindowFlags(Qt::WA_TranslucentBackground | Qt::WindowDoesNotAcceptFocus
                                    | Qt::WindowFullScreen | Qt::WindowStaysOnTopHint
                                    | Qt::FramelessWindowHint | Qt::Tool);
#ifdef Q_OS_WIN
        windowFlags |= Qt::WindowTransparentForInput;
#endif
        setFlags(windowFlags);
        setClearBeforeRendering(true);
        setColor(QColor(Qt::transparent));

        setResizeMode(QQuickView::SizeViewToRootObject);
        setSource(QUrl(QStringLiteral("qrc:/presenter/Presenter.qml")));

        const auto ourErrors = errors();
        for (const auto &error : ourErrors) {
            qCWarning(KDECONNECT_PLUGIN_PRESENT) << "error" << error.description() << error.url() << error.line() << error.column();
        }
    }
};

PresenterPlugin::PresenterPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_view(nullptr)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(500);
    m_timer->setSingleShot(true);
}

PresenterPlugin::~PresenterPlugin()
{
}

bool PresenterPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("stop"), false)) {
        delete m_view;
        m_view = nullptr;
        return true;
    }

    if (!m_view) {
        m_view = new PresenterView;
        m_xPos = 0.5f;
        m_yPos = 0.5f;
        m_view->showFullScreen();
        connect(this, &QObject::destroyed, m_view, &QObject::deleteLater);
        connect(m_timer, &QTimer::timeout, m_view, &QObject::deleteLater);
    }

    QSize screenSize = m_view->screen()->size();
    float ratio = float(screenSize.width())/screenSize.height();

    m_xPos += np.get<float>(QStringLiteral("dx"));
    m_yPos += np.get<float>(QStringLiteral("dy")) * ratio;
    m_xPos = qBound(0.f, m_xPos, 1.f);
    m_yPos = qBound(0.f, m_yPos, 1.f);

    m_timer->start();

    QQuickItem* object = m_view->rootObject();
    object->setProperty("xPos", m_xPos);
    object->setProperty("yPos", m_yPos);
    return true;
}

#include "presenterplugin.moc"

