/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <QObject>
#include <QHash>
#include <QString>

#include <KPluginMetaData>

#include "kdeconnectcore_export.h"

class Device;
class KdeConnectPlugin;
class KPluginFactory;

class KDECONNECTCORE_EXPORT PluginLoader
{

public:
    static PluginLoader* instance();

    QStringList getPluginList() const;
    KPluginMetaData getPluginInfo(const QString& name) const;
    KdeConnectPlugin* instantiatePluginForDevice(const QString& name, Device* device) const;

    QStringList incomingCapabilities() const;
    QStringList outgoingCapabilities() const;
    QSet<QString> pluginsForCapabilities(const QSet<QString>& incoming, const QSet<QString>& outgoing);

private:
    PluginLoader();

    QHash<QString, KPluginMetaData> plugins;
#ifdef SAILFISHOS
    QHash<QString, KPluginFactory*> pluginsFactories;
#endif
};

#endif
