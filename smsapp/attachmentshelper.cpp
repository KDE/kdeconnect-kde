/**
 * SPDX-FileCopyrightText: 2025 Yuki Joou <yukijoou@kemonomimi.gay>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "attachmentshelper.h"

#include <QFile>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>

#include <KLocalizedString>

// For ""_s
using namespace Qt::Literals::StringLiterals;

QObject *AttachmentsHelper::singletonProvider(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/)
{
    return new AttachmentsHelper();
}

void AttachmentsHelper::saveAttachment(const QString &mimetypeString, const QUrl &source)
{
    // TODO: Currently, this is always using the system's download folder.
    //       As the `share` plugin allows users to set a prefered destination for files,
    //       it'd be nice to respect it here.
    auto const downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    auto const filename = source.fileName();
    auto const mimetype = QMimeDatabase().mimeTypeForName(mimetypeString);

    QString outFile = QFileDialog::getSaveFileName(nullptr,
                                                   i18nc("Caption/title for attachment download/save window", "Download attachment"),
                                                   downloads + u"/"_s + filename,
                                                   mimetype.filterString());
    if (outFile.isNull() || outFile.isEmpty()) {
        // No file was selected, aborting
        return;
    }
    QFile::copy(source.toLocalFile(), outFile);
}
