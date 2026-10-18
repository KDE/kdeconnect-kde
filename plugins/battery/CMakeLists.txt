if(APPLE)
    set(kdeconnect_battery_SRCS
        batterymonitor-macos.cpp
    )
elseif(WIN32)
    set(kdeconnect_battery_SRCS
        batterymonitor-win.cpp
    )
else()
    set(kdeconnect_battery_SRCS
        batterymonitor-solid.cpp
    )
endif()

kdeconnect_add_plugin(kdeconnect_battery SOURCES batteryplugin.cpp ${kdeconnect_battery_SRCS})
target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt::DBus
    KF6::I18n
)
if(APPLE)
    target_link_libraries(kdeconnect_battery "-framework IOKit" "-framework CoreFoundation")
elseif(WIN32)
    target_link_libraries(kdeconnect_battery powrprof)
else()
    target_link_libraries(kdeconnect_battery KF6::Solid)
endif()
