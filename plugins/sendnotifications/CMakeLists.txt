find_package(KF5 REQUIRED COMPONENTS Notifications KCMUtils I18n IconThemes)

set(kdeconnect_sendnotifications_SRCS
    sendnotificationsplugin.cpp
    notificationslistener.cpp
    notifyingapplication.cpp
)

kdeconnect_add_plugin(kdeconnect_sendnotifications JSON kdeconnect_sendnotifications.json SOURCES ${kdeconnect_sendnotifications_SRCS})

target_link_libraries(kdeconnect_sendnotifications
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
    KF5::I18n
    KF5::IconThemes
    KF5::ConfigCore
)

#######################################
# Config

set( kdeconnect_sendnotifications_config_SRCS
     sendnotifications_config.cpp
     notifyingapplication.cpp
     notifyingapplicationmodel.cpp
)
ki18n_wrap_ui( kdeconnect_sendnotifications_config_SRCS sendnotifications_config.ui )

add_library(kdeconnect_sendnotifications_config MODULE ${kdeconnect_sendnotifications_config_SRCS} )
target_link_libraries( kdeconnect_sendnotifications_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF5::I18n
    KF5::KCMUtils
)

install( TARGETS kdeconnect_sendnotifications_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kdeconnect_sendnotifications_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
