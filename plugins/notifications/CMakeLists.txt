set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
)

add_library(kdeconnect_notifications MODULE ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications kdeconnectcore KF5::Service KF5::Notifications KF5::KDELibs4Support)

install(TARGETS kdeconnect_notifications DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_notifications.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(macros)
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
