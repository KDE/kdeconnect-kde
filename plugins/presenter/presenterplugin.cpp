/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "presenterplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDebug>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickView>
#include <QtGlobal>

#include "plugin_presenter_debug.h"
#include <QScreen>
#include <core/daemon.h>
#include <core/device.h>

K_PLUGIN_CLASS_WITH_JSON(PresenterPlugin, "kdeconnect_presenter.json")

class PresenterView : public QQuickView
{
public:
    PresenterView()
    {
        Qt::WindowFlags windowFlags = Qt::WindowFlags(Qt::WindowDoesNotAcceptFocus | Qt::WindowFullScreen | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint
                                                      | Qt::Tool | (int)(Qt::WA_TranslucentBackground));
#ifdef Q_OS_WIN
        windowFlags |= Qt::WindowTransparentForInput;
#endif
        setFlags(windowFlags);
        setColor(QColor(Qt::transparent));

        setResizeMode(QQuickView::SizeViewToRootObject);
        setSource(QUrl(QStringLiteral("qrc:/presenter/PresenterRedDot.qml")));

        const auto ourErrors = errors();
        for (const auto &error : ourErrors) {
            qCWarning(KDECONNECT_PLUGIN_PRESENTER) << "error" << error.description() << error.url() << error.line() << error.column();
        }
    }
};

PresenterPlugin::PresenterPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_view(nullptr)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(500);
    m_timer->setSingleShot(true);
}

void PresenterPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<bool>(QStringLiteral("stop"), false)) {
        delete m_view;
        m_view = nullptr;
        return;
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
    float ratio = float(screenSize.width()) / screenSize.height();

    m_xPos += np.get<float>(QStringLiteral("dx")) / 3;
    m_yPos += (np.get<float>(QStringLiteral("dy")) / 3) * ratio;
    m_xPos = qBound(0.f, m_xPos, 1.f);
    m_yPos = qBound(0.f, m_yPos, 1.f);

    m_timer->start();

    QQuickItem *object = m_view->rootObject();
    object->setProperty("xPos", m_xPos);
    object->setProperty("yPos", m_yPos);
}

#include "moc_presenterplugin.cpp"
#include "presenterplugin.moc"
