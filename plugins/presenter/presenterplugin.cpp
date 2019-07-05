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
#include <QLoggingCategory>
#include <QQuickView>
#include <QQmlError>
#include <QQuickItem>

#include <core/device.h>
#include <core/daemon.h>
#include <QScreen>

K_PLUGIN_CLASS_WITH_JSON(PresenterPlugin, "kdeconnect_presenter.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PRESENT, "kdeconnect.plugin.presenter")

class PresenterView : public QQuickView
{
public:
    PresenterView() {
        setFlags(flags() | Qt::WindowFlags(Qt::WA_TranslucentBackground));
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
    m_timer->setInterval(2000);
    m_timer->setSingleShot(true);
}

PresenterPlugin::~PresenterPlugin()
{
}

bool PresenterPlugin::receivePacket(const NetworkPacket& np)
{
    if (!m_view) {
        m_view = new PresenterView;
        m_view->showFullScreen();
        connect(this, &QObject::destroyed, m_view, &QObject::deleteLater);
        connect(m_timer, &QTimer::timeout, m_view, &QObject::deleteLater);
    }

    auto screenSize = m_view->screen()->size();
    auto ratio = screenSize.width()/screenSize.height();

    m_timer->start();

    auto object = m_view->rootObject();
    object->setProperty("xPos", np.get<qreal>(QStringLiteral("px"))/2 + 0.5);
    object->setProperty("yPos", np.get<qreal>(QStringLiteral("py"))/2 + 0.5);
    return true;
}

#include "presenterplugin.moc"

