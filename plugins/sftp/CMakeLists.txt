set(kdeconnect_sftp_SRCS
    mounter.cpp
    mountloop.cpp
    sftpplugin.cpp
)

kde4_add_plugin(kdeconnect_sftp ${kdeconnect_sftp_SRCS})

target_link_libraries(kdeconnect_sftp kdeconnectcore ${KDE4_KFILE_LIBS})

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
kde4_add_ui_files( kdeconnect_sftp_config_SRCS sftp_config.ui )

kde4_add_plugin( kdeconnect_sftp_config ${kdeconnect_sftp_config_SRCS} )
target_link_libraries( kdeconnect_sftp_config
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
)

install(TARGETS kdeconnect_sftp_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_sftp_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
