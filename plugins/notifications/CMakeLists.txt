find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
)

kdeconnect_add_plugin(kdeconnect_notifications JSON kdeconnect_notifications.json SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt5::DBus
    KF5::Service
    KF5::Notifications
)

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeconnect_notifications
    notificationsdbusinterface.h
    org.kde.kdeconnect.device.notifications.xml
    OPTIONS -a
)
generate_and_install_dbus_interface(
    kdeconnect_notifications
    notification.h
    org.kde.kdeconnect.device.notifications.notification.xml
    OPTIONS -a
)
