/*
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "notification.h"
#include "notification_p.h"

#include <algorithm>

#include <QDBusArgument>
#include <QDebug>
#include <QImageReader>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <KApplicationTrader>
#include <KConfig>
#include <KConfigGroup>
#include <KService>

#include "plugin_sendnotifications_debug.h"

using namespace NotificationManager;
using namespace Qt::StringLiterals;

QCache<uint, QImage> Notification::Private::s_imageCache = QCache<uint, QImage>{};

Notification::Private::Private()
{
}

Notification::Private::~Private()
{
    // The image cache is cleared by AbstractNotificationsModel::pendingRemovalTimer
}

QString Notification::Private::sanitize(const QString &text)
{
    // replace all \ns with <br/>
    QString t = text;

    t.replace(QLatin1String("\n"), QStringLiteral("<br/>"));
    // Now remove all inner whitespace (\ns are already <br/>s)
    t = std::move(t).simplified();
    // Replace <br></br> with just <br/> in case someone is mad enough to send that
    static const QRegularExpression brOpenCloseExpr(QStringLiteral("<br>\\s*</br>"));
    t.replace(brOpenCloseExpr, QLatin1String("<br/>"));
    // Replace single <br> with <br/> as QXmlStreamReader requires it
    t.replace(QStringLiteral("<br>"), QLatin1String("<br/>"));
    // Finally, check if we don't have multiple <br/>s following,
    // can happen for example when "\n       \n" is sent, this replaces
    // all <br/>s in succession with just one. Also remove extra whitespace after any newline
    static const QRegularExpression brExpr(QStringLiteral("<br/>(\\s|<br/>)+"));
    t.replace(brExpr, QLatin1String("<br/>"));
    // This fancy RegExp escapes every occurrence of & since QtQuick Text will blatantly cut off
    // text where it finds a stray ampersand.
    // Only &{apos, quot, gt, lt, amp}; as well as &#123 character references will be allowed
    static const QRegularExpression escapeExpr(QStringLiteral("&(?!(?:apos|quot|[gl]t|amp);|#)"));
    t.replace(escapeExpr, QLatin1String("&amp;"));

    // If the < or > is not followed or preceded by text or "/", escape it.
    // This nightmarish RegExp matches the < or >, then captures whatever is after/before it.
    // We then replace the match with the corresponding symbol, and add the captured bit back.
    static const QRegularExpression ltExpr(QStringLiteral("<([^\\/,a-zA-Z])"));
    t.replace(ltExpr, QLatin1String("&lt;\\1"));
    // Check for " in case the item is something like <img src="bla">
    static const QRegularExpression gtExpr(QStringLiteral("([^\\/,a-zA-Z,\"])>"));
    t.replace(gtExpr, QLatin1String("\\1&gt;"));

    // Don't bother adding some HTML structure if the body is now empty
    if (t.isEmpty()) {
        return t;
    }

    t = u"<html>" + std::move(t) + u"</html>";
    QXmlStreamReader r(t);
    QString result;
    QXmlStreamWriter out(&result);

    static constexpr std::array<QStringView, 10> allowedTags{u"b", u"i", u"u", u"img", u"a", u"html", u"br", u"table", u"tr", u"td"};

    out.writeStartDocument();
    while (!r.atEnd()) {
        r.readNext();

        if (r.tokenType() == QXmlStreamReader::StartElement) {
            const QStringView name = r.name();
            if (std::ranges::find(allowedTags, name) == allowedTags.end()) {
                continue;
            }
            out.writeStartElement(name);
            if (name == QLatin1String("img")) {
                const QString src = r.attributes().value("src").toString();
                const QStringView alt = r.attributes().value("alt");

                const QUrl url(src);
                if (url.isLocalFile()) {
                    out.writeAttribute(QStringLiteral("src"), src);
                } else {
                    // image denied for security reasons! Do not copy the image src here!
                }

                out.writeAttribute(u"alt", alt);
            }
            if (name == QLatin1Char('a')) {
                out.writeAttribute(u"href", r.attributes().value("href"));
            }
        }

        if (r.tokenType() == QXmlStreamReader::EndElement) {
            const QStringView name = r.name();
            if (std::ranges::find(allowedTags, name) == allowedTags.end()) {
                continue;
            }
            out.writeEndElement();
        }

        if (r.tokenType() == QXmlStreamReader::Characters) {
            out.writeCharacters(r.text()); // this auto escapes chars -> HTML entities
        }
    }
    out.writeEndDocument();

    if (r.hasError()) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS)
            << "Notification to send to backend contains invalid XML: " << r.errorString() << "line" << r.lineNumber() << "col" << r.columnNumber();
    }

    // The Text.StyledText format handles only html3.2 stuff and &apos; is html4 stuff
    // so we need to replace it here otherwise it will not render at all.
    result.replace(QLatin1String("&apos;"), QChar(u'\''));

    return result;
}

QImage Notification::Private::decodeNotificationSpecImageHint(const QDBusArgument &arg)
{
    int width, height, rowStride, hasAlpha, bitsPerSample, channels;
    QByteArray pixels;
    char *ptr;
    char *end;

    if (arg.currentType() != QDBusArgument::StructureType) {
        return QImage();
    }
    arg.beginStructure();
    arg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> pixels;
    arg.endStructure();

#define SANITY_CHECK(condition)                                                                                                                                \
    if (!(condition)) {                                                                                                                                        \
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Image decoding sanity check failed on" << #condition;                                               \
        return QImage();                                                                                                                                       \
    }

    SANITY_CHECK(width > 0);
    SANITY_CHECK(width < 2048);
    SANITY_CHECK(height > 0);
    SANITY_CHECK(height < 2048);
    SANITY_CHECK(rowStride > 0);

#undef SANITY_CHECK

    auto copyLineRGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 3;
        for (; src != end; ++dst, src += 3) {
            *dst = qRgb(src[0], src[1], src[2]);
        }
    };

    auto copyLineARGB32 = [](QRgb *dst, const char *src, int width) {
        const char *end = src + width * 4;
        for (; src != end; ++dst, src += 4) {
            *dst = qRgba(src[0], src[1], src[2], src[3]);
        }
    };

    QImage::Format format = QImage::Format_Invalid;
    void (*fcn)(QRgb *, const char *, int) = nullptr;
    if (bitsPerSample == 8) {
        if (channels == 4) {
            format = QImage::Format_ARGB32;
            fcn = copyLineARGB32;
        } else if (channels == 3) {
            format = QImage::Format_RGB32;
            fcn = copyLineRGB32;
        }
    }
    if (format == QImage::Format_Invalid) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS)
            << "Unsupported image format (hasAlpha:" << hasAlpha << "bitsPerSample:" << bitsPerSample << "channels:" << channels << ")";
        return QImage();
    }

    QImage image(width, height, format);
    ptr = pixels.data();
    end = ptr + pixels.length();
    for (int y = 0; y < height; ++y, ptr += rowStride) {
        if (ptr + channels * width > end) {
            qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Image data is incomplete. y:" << y << "height:" << height;
            break;
        }
        fcn((QRgb *)image.scanLine(y), ptr, width);
    }

    return image;
}

void Notification::Private::sanitizeImage(QImage &image)
{
    if (image.isNull()) {
        return;
    }

    const QSize max = maximumImageSize();
    if (image.size().width() > max.width() || image.size().height() > max.height()) {
        image = image.scaled(max, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

void Notification::Private::loadImagePath(const QString &path)
{
    // image_path and appIcon should either be a URL with file scheme or the name of a themed icon.
    // We're lenient and also allow local paths.

    s_imageCache.remove(id); // clear
    icon.clear();

    QUrl imageUrl;
    if (path.startsWith(QLatin1Char('/'))) {
        imageUrl = QUrl::fromLocalFile(path);
    } else if (path.contains(QLatin1Char('/'))) { // bad heuristic to detect a URL
        imageUrl = QUrl(path);

        if (!imageUrl.isLocalFile()) {
            qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Refused to load image from" << path << "which isn't a valid local location.";
            return;
        }
    }

    if (!imageUrl.isValid()) {
        // try icon path instead;
        icon = path;
        return;
    }

    QImageReader reader(imageUrl.toLocalFile());
    reader.setAutoTransform(true);

    if (QSize imageSize = reader.size(); imageSize.isValid()) {
        if (imageSize.width() > maximumImageSize().width() || imageSize.height() > maximumImageSize().height()) {
            imageSize = imageSize.scaled(maximumImageSize(), Qt::KeepAspectRatio);
            reader.setScaledSize(imageSize);
        }
        s_imageCache.insert(id, new QImage(reader.read()), imageSize.width() * imageSize.height());
    }
}

QString Notification::Private::defaultComponentName()
{
    // NOTE Keep in sync with KNotification
    return QStringLiteral("plasma_workspace");
}

constexpr QSize Notification::Private::maximumImageSize()
{
    return QSize(256, 256);
}

KService::Ptr Notification::Private::serviceForDesktopEntry(const QString &desktopEntry)
{
    if (desktopEntry.isEmpty()) {
        return {};
    }

    KService::Ptr service;

    if (desktopEntry.startsWith(QLatin1Char('/'))) {
        service = KService::serviceByDesktopPath(desktopEntry);
    } else {
        service = KService::serviceByDesktopName(desktopEntry);
    }

    if (!service) {
        const QString lowerDesktopEntry = desktopEntry.toLower();
        service = KService::serviceByDesktopName(lowerDesktopEntry);
    }

    // Try if it's a renamed flatpak
    if (!service) {
        const QString desktopId = desktopEntry + QLatin1String(".desktop");

        const auto services = KApplicationTrader::query([&desktopId](const KService::Ptr &app) -> bool {
            const QStringList renamedFrom = app->property<QStringList>(QStringLiteral("X-Flatpak-RenamedFrom"));
            return renamedFrom.contains(desktopId);
        });

        if (!services.isEmpty()) {
            service = services.first();
        }
    }

    // Try snap instance name.
    if (!service) {
        const auto services = KApplicationTrader::query([&desktopEntry](const KService::Ptr &app) -> bool {
            const QString snapInstanceName = app->property<QString>(QStringLiteral("X-SnapInstanceName"));
            return desktopEntry.compare(snapInstanceName, Qt::CaseInsensitive) == 0;
        });

        if (!services.isEmpty()) {
            service = services.first();
        }
    }

    return service;
}

void Notification::Private::setDesktopEntry(const QString &desktopEntry)
{
    QString serviceName;

    configurableService = false;

    KService::Ptr service = serviceForDesktopEntry(desktopEntry);
    if (service) {
        this->desktopEntry = service->desktopEntryName();
        serviceName = service->name();
        applicationIconName = service->icon();
        configurableService = !service->noDisplay();
    }

    const bool isDefaultEvent = (notifyRcName == defaultComponentName());
    configurableNotifyRc = false;
    if (!notifyRcName.isEmpty()) {
        // Check whether the application actually has notifications we can configure
        KConfig config(notifyRcName + QStringLiteral(".notifyrc"), KConfig::NoGlobals);

        QStringList configSources =
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications6/%1.notifyrc").arg(notifyRcName));
        // Keep compatibility with KF5 applications
        if (configSources.isEmpty()) {
            configSources = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications6/%1.notifyrc").arg(notifyRcName));
        }
        // `QStandardPaths` follows the order of precedence given by `$XDG_DATA_DIRS
        // (more priority goest first), but for `addConfigSources() it is the opposite
        std::reverse(configSources.begin(), configSources.end());
        config.addConfigSources(configSources);

        KConfigGroup globalGroup(&config, u"Global"_s);

        const QString iconName = globalGroup.readEntry("IconName");

        // also only overwrite application icon name for non-default events (or if we don't have a service icon)
        if (!iconName.isEmpty() && (!isDefaultEvent || applicationIconName.isEmpty())) {
            applicationIconName = iconName;
        }

        const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));
        configurableNotifyRc = !config.groupList().filter(regexp).isEmpty();
    }

    // For default events we try to show the application name from the desktop entry if possible
    // This will have us show e.g. "Dr Konqi" instead of generic "Plasma Desktop"
    // The application may not send an applicationName. Use the name from the desktop entry then
    if ((isDefaultEvent || applicationName.isEmpty()) && !serviceName.isEmpty()) {
        applicationName = serviceName;
    }
}

void Notification::Private::processHints(const QVariantMap &hints)
{
    auto end = hints.end();

    notifyRcName = hints.value(QStringLiteral("x-kde-appname")).toString();

    setDesktopEntry(hints.value(QStringLiteral("desktop-entry")).toString());

    // Special override for KDE Connect since the notification is sent by kdeconnectd
    // but actually comes from a different app on the phone
    const QString applicationDisplayName = hints.value(QStringLiteral("x-kde-display-appname")).toString();
    if (!applicationDisplayName.isEmpty()) {
        applicationName = applicationDisplayName;
    }

    originName = hints.value(QStringLiteral("x-kde-origin-name")).toString();

    eventId = hints.value(QStringLiteral("x-kde-eventId")).toString();
    xdgTokenAppId = hints.value(QStringLiteral("x-kde-xdgTokenAppId")).toString();

    bool ok;
    const int urgency = hints.value(QStringLiteral("urgency")).toInt(&ok); // DBus type is actually "byte"
    if (ok) {
        setUrgency(urgency);
    }

    resident = hints.value(QStringLiteral("resident")).toBool();
    transient = hints.value(QStringLiteral("transient")).toBool();

    userActionFeedback = hints.value(QStringLiteral("x-kde-user-action-feedback")).toBool();
    if (userActionFeedback) {
        // A confirmation of an explicit user interaction is assumed to have been seen by the user.
        read = true;
    }

    urls = QUrl::fromStringList(hints.value(QStringLiteral("x-kde-urls")).toStringList());

    replyPlaceholderText = hints.value(QStringLiteral("x-kde-reply-placeholder-text")).toString();
    replySubmitButtonText = hints.value(QStringLiteral("x-kde-reply-submit-button-text")).toString();
    replySubmitButtonIconName = hints.value(QStringLiteral("x-kde-reply-submit-button-icon-name")).toString();

    category = hints.value(QStringLiteral("category")).toString();

    // Underscored hints was in use in version 1.1 of the spec but has been
    // replaced by dashed hints in version 1.2. We need to support it for
    // users of the 1.2 version of the spec.
    auto it = hints.find(QStringLiteral("image-data"));
    if (it == end) {
        it = hints.find(QStringLiteral("image_data"));
    }
    if (it == end) {
        // This hint was in use in version 1.0 of the spec but has been
        // replaced by "image_data" in version 1.1. We need to support it for
        // users of the 1.0 version of the spec.
        it = hints.find(QStringLiteral("icon_data"));
    }

    QImage *image = nullptr;
    if (it != end) {
        QImage imageFromHint(decodeNotificationSpecImageHint(it->value<QDBusArgument>()));
        if (!imageFromHint.isNull()) {
            Q_ASSERT_X(imageFromHint.width() > 0 && imageFromHint.height() > 0,
                       Q_FUNC_INFO,
                       qPrintable(QStringLiteral("%1 %2").arg(QString::number(imageFromHint.width()), QString::number(imageFromHint.height()))));
            sanitizeImage(imageFromHint);
            image = new QImage(imageFromHint);
            s_imageCache.insert(id, image, imageFromHint.width() * imageFromHint.height());
        }
    }

    if (!image) {
        it = hints.find(QStringLiteral("image-path"));
        if (it == end) {
            it = hints.find(QStringLiteral("image_path"));
        }

        if (it != end) {
            loadImagePath(it->toString());
        }
    }

    this->hints = hints;
}

void Notification::Private::setUrgency(int urgency)
{
    this->urgency = urgency;

    // Critical notifications must not time out
    // TODO should we really imply this here and not on the view side?
    // are there usecases for critical but can expire?
    // "critical updates available"?
    if (urgency == 3) {
        timeout = 0;
    }
}

Notification::Notification(uint id)
    : d(new Private())
{
    d->id = id;
    d->created = QDateTime::currentDateTimeUtc();
}

Notification::Notification(const Notification &other)
    : d(new Private(*other.d))
{
}

Notification::Notification(Notification &&other) noexcept
    : d(other.d)
{
    other.d = nullptr;
}

Notification &Notification::operator=(const Notification &other)
{
    *d = *other.d;
    return *this;
}

Notification &Notification::operator=(Notification &&other) noexcept
{
    d = other.d;
    other.d = nullptr;
    return *this;
}

Notification::~Notification()
{
    delete d;
}

uint Notification::id() const
{
    return d->id;
}

QString Notification::dBusService() const
{
    return d->dBusService;
}

void Notification::setDBusService(const QString &dBusService)
{
    d->dBusService = dBusService;
}

QDateTime Notification::created() const
{
    return d->created;
}

void Notification::setCreated(const QDateTime &created)
{
    d->created = created;
}

QDateTime Notification::updated() const
{
    return d->updated;
}

void Notification::resetUpdated()
{
    d->updated = QDateTime::currentDateTimeUtc();
}

bool Notification::read() const
{
    return d->read;
}

void Notification::setRead(bool read)
{
    d->read = read;
}

QString Notification::summary() const
{
    return d->summary;
}

void Notification::setSummary(const QString &summary)
{
    d->summary = summary;
}

QString Notification::body() const
{
    return d->body;
}

void Notification::setBody(const QString &body)
{
    d->rawBody = body;
    d->body = Private::sanitize(body.trimmed());
}

QString Notification::rawBody() const
{
    return d->rawBody;
}

QString Notification::icon() const
{
    return d->icon;
}

void Notification::setIcon(const QString &icon)
{
    d->loadImagePath(icon);
}

QImage Notification::image() const
{
    if (d->s_imageCache.contains(d->id)) {
        return *d->s_imageCache.object(d->id);
    }
    return QImage();
}

void Notification::setImage(const QImage &image)
{
    d->s_imageCache.insert(d->id, new QImage(image));
}

QString Notification::desktopEntry() const
{
    return d->desktopEntry;
}

void Notification::setDesktopEntry(const QString &desktopEntry)
{
    d->setDesktopEntry(desktopEntry);
}

QString Notification::notifyRcName() const
{
    return d->notifyRcName;
}

QString Notification::eventId() const
{
    return d->eventId;
}

QString Notification::applicationName() const
{
    return d->applicationName;
}

void Notification::setApplicationName(const QString &applicationName)
{
    d->applicationName = applicationName;
}

QString Notification::applicationIconName() const
{
    return d->applicationIconName;
}

void Notification::setApplicationIconName(const QString &applicationIconName)
{
    d->applicationIconName = applicationIconName;
}

QString Notification::originName() const
{
    return d->originName;
}

QStringList Notification::actionNames() const
{
    return d->actionNames;
}

QStringList Notification::actionLabels() const
{
    return d->actionLabels;
}

bool Notification::hasDefaultAction() const
{
    return d->hasDefaultAction;
}

QString Notification::defaultActionLabel() const
{
    // Most apps don't expect the default action be visible to the user.
    // For KDE apps we can assume KNotification does something reasonable.
    if (!d->notifyRcName.isEmpty() && d->notifyRcName != Private::defaultComponentName()) {
        return d->defaultActionLabel;
    } else {
        return QString(); // Let the UI pick a sensible default.
    }
}

void Notification::setActions(const QStringList &actions)
{
    if (actions.count() % 2 != 0) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "List of actions must contain an even number of items, tried to set actions to" << actions;
        return;
    }

    d->hasDefaultAction = false;
    d->hasConfigureAction = false;
    d->hasReplyAction = false;

    QStringList names;
    QStringList labels;

    for (int i = 0; i < actions.count(); i += 2) {
        const QString &name = actions.at(i);
        const QString &label = actions.at(i + 1);

        if (!d->hasDefaultAction && name == QLatin1String("default")) {
            d->hasDefaultAction = true;
            d->defaultActionLabel = label;
            continue;
        }

        if (!d->hasConfigureAction && name == QLatin1String("settings")) {
            d->hasConfigureAction = true;
            d->configureActionLabel = label;
            continue;
        }

        if (!d->hasReplyAction && name == QLatin1String("inline-reply")) {
            d->hasReplyAction = true;
            d->replyActionLabel = label;
            continue;
        }

        names << name;
        labels << label;
    }

    d->actionNames = names;
    d->actionLabels = labels;
}

QList<QUrl> Notification::urls() const
{
    return d->urls;
}

void Notification::setUrls(const QList<QUrl> &urls)
{
    d->urls = urls;
}

int Notification::urgency() const
{
    return d->urgency;
}

bool Notification::userActionFeedback() const
{
    return d->userActionFeedback;
}

int Notification::timeout() const
{
    return d->timeout;
}

void Notification::setTimeout(int timeout)
{
    d->timeout = timeout;
}

bool Notification::configurable() const
{
    return d->hasConfigureAction || d->configurableNotifyRc || d->configurableService;
}

QString Notification::configureActionLabel() const
{
    return d->configureActionLabel;
}

bool Notification::hasReplyAction() const
{
    return d->hasReplyAction;
}

QString Notification::replyActionLabel() const
{
    return d->replyActionLabel;
}

QString Notification::replyPlaceholderText() const
{
    return d->replyPlaceholderText;
}

QString Notification::replySubmitButtonText() const
{
    return d->replySubmitButtonText;
}

QString Notification::replySubmitButtonIconName() const
{
    return d->replySubmitButtonIconName;
}

QString Notification::category() const
{
    return d->category;
}

bool Notification::expired() const
{
    return d->expired;
}

void Notification::setExpired(bool expired)
{
    d->expired = expired;
}

bool Notification::dismissed() const
{
    return d->dismissed;
}

void Notification::setDismissed(bool dismissed)
{
    d->dismissed = dismissed;
}

bool Notification::resident() const
{
    return d->resident;
}

void Notification::setResident(bool resident)
{
    d->resident = resident;
}

bool Notification::transient() const
{
    return d->transient;
}

void Notification::setTransient(bool transient)
{
    d->transient = transient;
}

QVariantMap Notification::hints() const
{
    return d->hints;
}

void Notification::setHints(const QVariantMap &hints)
{
    d->hints = hints;
}

void Notification::processHints(const QVariantMap &hints)
{
    d->processHints(hints);
}

bool Notification::wasAddedDuringInhibition() const
{
    return d->wasAddedDuringInhibition;
}

void Notification::setWasAddedDuringInhibition(bool value)
{
    d->wasAddedDuringInhibition = value;
}
