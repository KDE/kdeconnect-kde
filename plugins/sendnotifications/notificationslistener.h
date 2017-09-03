/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusArgument>
#include <core/device.h>
#include <QBuffer>
#include <QFile>

class KdeConnectPlugin;
class Notification;
struct NotifyingApplication;

class NotificationsListener : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit NotificationsListener(KdeConnectPlugin* aPlugin);
    ~NotificationsListener() override;

protected:
    KdeConnectPlugin* m_plugin;
    QHash<QString, NotifyingApplication> m_applications;

    // virtual helper function to make testing possible (QDBusArgument can not
    // be injected without making a DBUS-call):
    virtual bool parseImageDataArgument(const QVariant& argument, int& width,
                                        int& height, int& rowStride, int& bitsPerSample,
                                        int& channels, bool& hasAlpha,
                                        QByteArray& imageData) const;
    QSharedPointer<QIODevice> iconForImageData(const QVariant& argument) const;
    QSharedPointer<QIODevice> iconForIconName(const QString& iconName) const;

public Q_SLOTS:
    Q_SCRIPTABLE uint Notify(const QString&, uint, const QString&,
                             const QString&, const QString&,
                             const QStringList&, const QVariantMap&, int);

private Q_SLOTS:
    void loadApplications();

private:
    void setTranslatedAppName();
    QString m_translatedAppName;
};
