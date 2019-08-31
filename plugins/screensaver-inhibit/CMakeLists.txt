if(APPLE)
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin-macos.cpp
    )
elseif(WIN32)
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin-win.cpp
    )
else()
    set(kdeconnect_screensaver_inhibit_SRCS
        screensaverinhibitplugin.cpp
    )
endif()

kdeconnect_add_plugin(kdeconnect_screensaver_inhibit JSON kdeconnect_screensaver_inhibit.json SOURCES ${kdeconnect_screensaver_inhibit_SRCS})

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore)
if(NOT APPLE AND NOT WIN32)
    target_link_libraries(kdeconnect_screensaver_inhibit
        Qt5::DBus
        KF5::I18n
    )
endif()

