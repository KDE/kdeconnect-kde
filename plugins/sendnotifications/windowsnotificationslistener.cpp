/**
 * SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "windowsnotificationslistener.h"

#include <QImage>

#include <core/kdeconnectplugin.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

#include "plugin_sendnotifications_debug.h"

using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Notifications;
using namespace Windows::UI::Notifications::Management;
using namespace Windows::Storage::Streams;

WindowsNotificationsListener::WindowsNotificationsListener(KdeConnectPlugin *aPlugin)
    : NotificationsListener(aPlugin)
{
    setupWindowsUserNotificationListener();
}

WindowsNotificationsListener::~WindowsNotificationsListener()
{
    UserNotificationListener::Current().NotificationChanged(m_notificationEventToken);
}

void WindowsNotificationsListener::setupWindowsUserNotificationListener()
{
    // Register the notification listener
    const UserNotificationListenerAccessStatus accessStatus = UserNotificationListener::Current().RequestAccessAsync().get();
    if (accessStatus != UserNotificationListenerAccessStatus::Allowed) {
        return;
    }

    // Register the event handler for notifications
    m_notificationEventToken = UserNotificationListener::Current().NotificationChanged({this, &WindowsNotificationsListener::onNotificationChanged});
}

void WindowsNotificationsListener::onNotificationChanged(const UserNotificationListener &sender, const UserNotificationChangedEventArgs &args)
{
    // Get the notification from the event arguments
    const UserNotificationChangedKind changeKind = args.ChangeKind();
    if (changeKind == UserNotificationChangedKind::Removed) {
        return;
    }

    const UserNotification userNotification = sender.GetNotification(args.UserNotificationId());
    if (!userNotification) {
        return;
    }

    const AppDisplayInfo appDisplayInfo = userNotification.AppInfo().DisplayInfo();
    const std::wstring_view appDisplayName = appDisplayInfo.DisplayName();
    const QString appName = QString::fromWCharArray(appDisplayName.data(), appDisplayName.size());
    if (!checkApplicationName(appName)) {
        return;
    }

    const winrt::Windows::UI::Notifications::Notification notification = userNotification.Notification();
    const NotificationBinding toastBinding = notification.Visual().GetBinding(KnownNotificationBindings::ToastGeneric());
    if (!toastBinding) {
        return;
    }
    const auto textElements = toastBinding.GetTextElements();
    if (textElements.Size() == 0) {
        return;
    }

    std::wstring ticker = static_cast<std::wstring>(textElements.GetAt(0).Text());
    if (m_plugin->config()->getBool(QStringLiteral("generalIncludeBody"), true)) {
        for (unsigned i = 1; i < textElements.Size(); ++i) {
            ticker += L"\n" + textElements.GetAt(i).Text();
        }
    }

    const QString content = QString::fromStdWString(ticker);
    if (checkIsInBlacklist(appName, content)) {
        return;
    }

    static unsigned id = 0;
    if (id == std::numeric_limits<unsigned>::max()) {
        id = 0;
    }
    NetworkPacket np(PACKET_TYPE_NOTIFICATION,
                     {{QStringLiteral("id"), id++},
                      {QStringLiteral("appName"), appName},
                      {QStringLiteral("ticker"), content},
                      {QStringLiteral("isClearable"), true}}); // A Windows notification is always clearable

    if (m_plugin->config()->getBool(QStringLiteral("generalSynchronizeIcons"), true)) {
        const RandomAccessStreamReference appLogoStreamReference = appDisplayInfo.GetLogo(Size(64, 64));
        if (appLogoStreamReference) { // Can be false when Package.appxmanifest doesn't contain icons
            // Read the logo stream into a buffer
            IRandomAccessStreamWithContentType logoStream = appLogoStreamReference.OpenReadAsync().get(); // TODO Port to coroutine
            DataReader reader(logoStream);
            reader.LoadAsync(static_cast<uint32_t>(logoStream.Size())).get();
            std::vector<unsigned char> bufferArray;
            bufferArray.resize(reader.UnconsumedBufferLength());
            if (!bufferArray.empty()) {
                reader.ReadBytes({bufferArray.data(), bufferArray.data() + bufferArray.size()});

                QImage image;
                if (image.loadFromData(bufferArray.data(), static_cast<int>(bufferArray.size()))) {
                    // Write the logo buffer to the QIODevice
                    QSharedPointer<QIODevice> iconSource = iconFromQImage(image);
                    if (iconSource) {
                        np.setPayload(iconSource, iconSource->size());
                    }
                }
            }
        }
    }

    m_plugin->sendPacket(np);
}

#include "moc_windowsnotificationslistener.cpp"
