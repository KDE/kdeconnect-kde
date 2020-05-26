set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_battery_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_BATTERY CATEGORY_NAME kdeconnect.plugin.battery
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin battery)")

set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    batterydbusinterface.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_battery JSON kdeconnect_battery.json SOURCES ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt5::DBus
    KF5::Solid
    KF5::I18n
)
