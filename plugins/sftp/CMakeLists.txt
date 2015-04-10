find_package(KF5 REQUIRED COMPONENTS Notifications KIO)

set(kdeconnect_sftp_SRCS
    mounter.cpp
    mountloop.cpp
    sftpplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_sftp JSON kdeconnect_sftp.json SOURCES ${kdeconnect_sftp_SRCS})

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

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeconnect_sftp
    sftpplugin.h
    org.kde.kdeconnect.device.sftp.xml
    OPTIONS -a
)
