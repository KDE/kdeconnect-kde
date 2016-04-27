find_package(KF5 REQUIRED COMPONENTS Notifications KCMUtils I18n IconThemes Config)

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
    notificationslistener.cpp
    notifyingapplication.cpp
)

kdeconnect_add_plugin(kdeconnect_notifications JSON kdeconnect_notifications.json SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
    KF5::I18n
    KF5::IconThemes
    KF5::ConfigCore
)

#######################################
# Config

set( kdeconnect_notifications_config_SRCS
     notifications_config.cpp
     notifyingapplication.cpp
     notifyingapplicationmodel.cpp
)
ki18n_wrap_ui( kdeconnect_notifications_config_SRCS notifications_config.ui )

add_library(kdeconnect_notifications_config MODULE ${kdeconnect_notifications_config_SRCS} )
target_link_libraries( kdeconnect_notifications_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF5::I18n
    KF5::KCMUtils
)

install( TARGETS kdeconnect_notifications_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kdeconnect_notifications_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
