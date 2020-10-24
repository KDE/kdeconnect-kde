/*
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
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

