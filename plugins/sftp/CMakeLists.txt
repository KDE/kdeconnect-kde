set(kdeconnect_sftp_SRCS
    mounter.cpp
    mountloop.cpp
    sftpplugin.cpp
)

add_library(kdeconnect_sftp MODULE ${kdeconnect_sftp_SRCS})

target_link_libraries(kdeconnect_sftp kdeconnectcore)

install(TARGETS kdeconnect_sftp DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_sftp.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(macros)

generate_and_install_dbus_interface(
    kdeconnect_sftp
    sftpplugin.h
    org.kde.kdeconnect.device.sftp.xml
    OPTIONS -a
)

#######################################
# Config

set( kdeconnect_sftp_config_SRCS sftp_config.cpp )
qt5_wrap_ui( kdeconnect_sftp_config_SRCS sftp_config.ui )

add_library(kdeconnect_sftp_config MODULE ${kdeconnect_sftp_config_SRCS} )
target_link_libraries( kdeconnect_sftp_config
    
    KF5::KDELibs4Support
    KF5::KIOWidgets
)

install(TARGETS kdeconnect_sftp_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_sftp_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
