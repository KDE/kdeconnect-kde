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
