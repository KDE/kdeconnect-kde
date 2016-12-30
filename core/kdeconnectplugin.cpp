/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "kdeconnectplugin.h"

#include "core_debug.h"

struct KdeConnectPluginPrivate
{
    Device* mDevice;
    QString mPluginName;
    QSet<QString> mOutgoingCapabilties;
    KdeConnectPluginConfig* mConfig;
};

KdeConnectPlugin::KdeConnectPlugin(QObject* parent, const QVariantList& args)
    : QObject(parent)
    , d(new KdeConnectPluginPrivate)
{
    d->mDevice = qvariant_cast< Device* >(args.at(0));
    d->mPluginName = args.at(1).toString();
    d->mOutgoingCapabilties = args.at(2).toStringList().toSet();
    d->mConfig = nullptr;
}

KdeConnectPluginConfig* KdeConnectPlugin::config() const
{
    //Create on demand, because not every plugin will use it
    if (!d->mConfig) {
        d->mConfig = new KdeConnectPluginConfig(d->mDevice->id(), d->mPluginName);
    }
    return d->mConfig;
}

KdeConnectPlugin::~KdeConnectPlugin()
{
    if (d->mConfig) {
        delete d->mConfig;
    }
}

const Device* KdeConnectPlugin::device()
{
    return d->mDevice;
}

Device const* KdeConnectPlugin::device() const
{
    return d->mDevice;
}

bool KdeConnectPlugin::sendPackage(NetworkPackage& np) const
{
    if(!d->mOutgoingCapabilties.contains(np.type())) {
        qCWarning(KDECONNECT_CORE) << metaObject()->className() << "tried to send an unsupported package type" << np.type() << ". Supported:" << d->mOutgoingCapabilties;
        return false;
    }
//     qCWarning(KDECONNECT_CORE) << metaObject()->className() << "sends" << np.type() << ". Supported:" << d->mOutgoingTypes;
    return d->mDevice->sendPackage(np);
}

QString KdeConnectPlugin::dbusPath() const
{
    return {};
}
