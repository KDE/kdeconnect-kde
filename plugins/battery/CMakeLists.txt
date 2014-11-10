set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    batterydbusinterface.cpp
)

kde4_add_plugin(kdeconnect_battery ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery kdeconnectcore ${KDE4_KDEUI_LIBS})

install(TARGETS kdeconnect_battery DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_battery.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(DbusInterfaceMacros)

generate_and_install_dbus_interface(
    kdeconnect_battery
    batterydbusinterface.h
    org.kde.kdeconnect.device.battery.xml
    OPTIONS -a
)
