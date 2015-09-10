find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
    notificationslistener.cpp
)

kdeconnect_add_plugin(kdeconnect_notifications JSON kdeconnect_notifications.json SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
)
