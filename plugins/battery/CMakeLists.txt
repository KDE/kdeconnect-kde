set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    batterydbusinterface.cpp
)

kdeconnect_add_plugin(kdeconnect_battery JSON kdeconnect_battery.json SOURCES ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
    KF5::Service
    KF5::Notifications
)

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeconnect_battery
    batterydbusinterface.h
    org.kde.kdeconnect.device.battery.xml
    OPTIONS -a
)
