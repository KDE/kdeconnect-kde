/**
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kcmplugin/kdeconnectpluginkcm.h"

class QMenu;
class QStandardItemModel;

class RunCommandConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    RunCommandConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);

    void save() override;
    void load() override;
    void defaults() override;

private:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void exportCommands();
    void importCommands();

    void addSuggestedCommand(QMenu *menu, const QString &name, const QString &command);
    void insertRow(int i, const QString &name, const QString &command);
    void insertEmptyRow();

    QStandardItemModel *m_entriesModel;
};
