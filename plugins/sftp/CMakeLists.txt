set(kdeconnect_sftp_SRCS
    mounter.cpp
    mountloop.cpp
    sftpplugin.cpp
)

add_library(kdeconnect_sftp MODULE ${kdeconnect_sftp_SRCS})

target_link_libraries(kdeconnect_sftp
    kdeconnectcore
    Qt5::DBus
    KF5::Service
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::IconThemes
    KF5::KIOWidgets
    KF5::Notifications
)

install(TARGETS kdeconnect_sftp DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_sftp.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeconnect_sftp
    sftpplugin.h
    org.kde.kdeconnect.device.sftp.xml
    OPTIONS -a
)

#######################################
# Config

set( kdeconnect_sftp_config_SRCS sftp_config.cpp )
ki18n_wrap_ui( kdeconnect_sftp_config_SRCS sftp_config.ui )

add_library(kdeconnect_sftp_config MODULE ${kdeconnect_sftp_config_SRCS} )
target_link_libraries( kdeconnect_sftp_config
    KF5::I18n
    KF5::KIOWidgets
    KF5::KCMUtils
    KF5::IconThemes
    qca
)

install(TARGETS kdeconnect_sftp_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_sftp_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
