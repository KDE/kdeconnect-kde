find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_screensaver_inhibit_SRCS
    screensaverinhibitplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_screensaver_inhibit JSON kdeconnect_screensaver_inhibit.json SOURCES ${kdeconnect_screensaver_inhibit_SRCS})

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore 
    Qt5::DBus
    KF5::I18n
    KF5::Notifications
)
