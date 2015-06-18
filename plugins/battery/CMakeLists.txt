find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    batterydbusinterface.cpp
)

kdeconnect_add_plugin(kdeconnect_battery JSON kdeconnect_battery.json SOURCES ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
    KF5::Notifications
)
