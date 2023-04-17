/**
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RUNCOMMAND_CONFIG_H
#define RUNCOMMAND_CONFIG_H

#include "kcmplugin/kdeconnectpluginkcm.h"

class QMenu;
class QStandardItemModel;

class RunCommandConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    RunCommandConfig(QObject *parent, const QVariantList &);
    ~RunCommandConfig() override;

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private Q_SLOTS:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void exportCommands();
    void importCommands();

private:
    void addSuggestedCommand(QMenu *menu, const QString &name, const QString &command);
    void insertRow(int i, const QString &name, const QString &command);
    void insertEmptyRow();

    QStandardItemModel *m_entriesModel;
};

#endif // RUNCOMMAND_CONFIG_H
