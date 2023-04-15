set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_battery_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_BATTERY CATEGORY_NAME kdeconnect.plugin.battery
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin battery)")

set(kdeconnect_battery_SRCS
    batteryplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_battery SOURCES ${kdeconnect_battery_SRCS})

target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
