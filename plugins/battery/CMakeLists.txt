set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    batterydbusinterface.cpp
)

add_library(kdeconnect_battery MODULE ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
    KF5::Service
    KF5::Notifications
)

install(TARGETS kdeconnect_battery DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_battery.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(macros)

generate_and_install_dbus_interface(
    kdeconnect_battery
    batterydbusinterface.h
    org.kde.kdeconnect.device.battery.xml
    OPTIONS -a
)
