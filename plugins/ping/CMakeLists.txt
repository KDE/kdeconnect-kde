find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_ping_SRCS
    pingplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_ping JSON kdeconnect_ping.json SOURCES ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
    KF5::Service
    KF5::Notifications
)
