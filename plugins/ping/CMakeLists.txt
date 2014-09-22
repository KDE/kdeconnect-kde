set(kdeconnect_ping_SRCS
    pingplugin.cpp
)

add_library(kdeconnect_ping MODULE ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
    KF5::Service
    KF5::Notifications
)

install(TARGETS kdeconnect_ping DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_ping.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
