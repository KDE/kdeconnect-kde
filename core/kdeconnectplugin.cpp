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

struct KdeConnectPluginPrivate
{
    Device* mDevice;
    QSet<QString> mOutgoingTypes;

    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    // Since the plugins use their own memory, they need their own initializer in order to send encrypted packages
    QCA::Initializer init;
};

KdeConnectPlugin::KdeConnectPlugin(QObject* parent, const QVariantList& args)
    : QObject(parent)
    , d(new KdeConnectPluginPrivate)
{
    d->mDevice = qvariant_cast< Device* >(args.first());
    d->mOutgoingTypes = args.last().toStringList().toSet();
}

KdeConnectPlugin::~KdeConnectPlugin()
{
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
    if(!d->mOutgoingTypes.contains(np.type())) {
        qWarning() << metaObject()->className() << "tried to send an unsupported package type" << np.type() << ". Supported:" << d->mOutgoingTypes;
        return false;
    }
//     qWarning() << metaObject()->className() << "sends" << np.type() << ". Supported:" << d->mOutgoingTypes;
    return d->mDevice->sendPackage(np);
}
