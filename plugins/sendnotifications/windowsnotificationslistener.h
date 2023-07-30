/**
 * SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notificationslistener.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Notifications.Management.h>
#include <winrt/Windows.UI.Notifications.h>

class WindowsNotificationsListener : public NotificationsListener
{
    Q_OBJECT

public:
    explicit WindowsNotificationsListener(KdeConnectPlugin *aPlugin);
    ~WindowsNotificationsListener() override;

private:
    void setupWindowsUserNotificationListener();
    void onNotificationChanged(const winrt::Windows::UI::Notifications::Management::UserNotificationListener &sender,
                               const winrt::Windows::UI::Notifications::UserNotificationChangedEventArgs &args);

    winrt::event_token m_notificationEventToken;
};
