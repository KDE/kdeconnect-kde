/*
 * Copyright 2018 Nicolas Fella <nicolas.fella@gmx.de>
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

#include <QApplication>
#include <QCommandLineParser>
#include <QStyle>

#include <KCMultiDialog>
#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>
#include "kdeconnect-version.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KAboutData about(QStringLiteral("kdeconnect-settings"),
                     i18n("KDE Connect Settings"),
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     i18n("KDE Connect Settings"),
                     KAboutLicense::GPL,
                     i18n("(C) 2018-2020 Nicolas Fella"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("args"), i18n("Arguments for the config module"), QStringLiteral("args")));

    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    KDBusService dbusService(KDBusService::Unique);

    KCMultiDialog* dialog = new KCMultiDialog;
    dialog->addModule(QStringLiteral("kcm_kdeconnect"), {parser.value(QStringLiteral("args"))});

    auto style = dialog->style();
    dialog->setContentsMargins(style->pixelMetric(QStyle::PM_LayoutLeftMargin),
                               style->pixelMetric(QStyle::PM_LayoutTopMargin),
                               style->pixelMetric(QStyle::PM_LayoutRightMargin),
                               style->pixelMetric(QStyle::PM_LayoutBottomMargin));

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    app.setQuitOnLastWindowClosed(true);

    return app.exec();
}

