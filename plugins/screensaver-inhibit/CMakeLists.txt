set(kdeconnect_screensaver_inhibit_SRCS
    screensaverinhibitplugin.cpp
)

add_library(kdeconnect_screensaver_inhibit ${kdeconnect_screensaver_inhibit_SRCS})

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore 
    Qt5::DBus
    KF5::I18n
    KF5::Service
    KF5::Notifications
)

install(TARGETS kdeconnect_screensaver_inhibit DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_screensaver_inhibit.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
