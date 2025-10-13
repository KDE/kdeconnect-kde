/**
 * SPDX-FileCopyrightText: 2025 Yuki Joou <yukijoou@kemonomimi.gay>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ATTACHMENTSHELPER_H
#define ATTACHMENTSHELPER_H

#include <QJSEngine>
#include <QQmlEngine>

class AttachmentsHelper : public QObject
{
    Q_OBJECT
public:
    static QObject *singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);

    /**
     * Opens a save dialog for the user to pick where to save an attachment, and save it at
     * that location.
     */
    Q_INVOKABLE static void saveAttachment(const QString &mimetypeString, const QUrl &source);
};

#endif // ATTACHMENTSHELPER_H
