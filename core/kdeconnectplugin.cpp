/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectplugin.h"

#include "core_debug.h"

struct KdeConnectPluginPrivate {
    Device *m_device;
    QString m_pluginName;
    QSet<QString> m_outgoingCapabilties;
    KdeConnectPluginConfig *m_config;
    QString iconName;
};

KdeConnectPlugin::KdeConnectPlugin(QObject *parent, const QVariantList &args)
    : QObject(parent)
    , d(new KdeConnectPluginPrivate)
{
    d->m_device = qvariant_cast<Device *>(args.at(0));
    d->m_pluginName = args.at(1).toString();

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QStringList cap = args.at(2).toStringList();
    d->m_outgoingCapabilties = QSet(cap.begin(), cap.end());
#else
    d->m_outgoingCapabilties = args.at(2).toStringList().toSet();
#endif
    d->m_config = nullptr;
    d->iconName = args.at(3).toString();
}

KdeConnectPluginConfig *KdeConnectPlugin::config() const
{
    // Create on demand, because not every plugin will use it
    if (!d->m_config) {
        d->m_config = new KdeConnectPluginConfig(d->m_device->id(), d->m_pluginName);
    }
    return d->m_config;
}

KdeConnectPlugin::~KdeConnectPlugin()
{
    if (d->m_config) {
        delete d->m_config;
    }
}

const Device *KdeConnectPlugin::device()
{
    return d->m_device;
}

Device const *KdeConnectPlugin::device() const
{
    return d->m_device;
}

bool KdeConnectPlugin::sendPacket(NetworkPacket &np) const
{
    if (!d->m_outgoingCapabilties.contains(np.type())) {
        qCWarning(KDECONNECT_CORE) << metaObject()->className() << "tried to send an unsupported packet type" << np.type()
                                   << ". Supported:" << d->m_outgoingCapabilties;
        return false;
    }
    //     qCWarning(KDECONNECT_CORE) << metaObject()->className() << "sends" << np.type() << ". Supported:" << d->mOutgoingTypes;
    return d->m_device->sendPacket(np);
}

QString KdeConnectPlugin::dbusPath() const
{
    return {};
}

QString KdeConnectPlugin::iconName() const
{
    return d->iconName;
}
