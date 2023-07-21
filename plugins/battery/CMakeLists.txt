kdeconnect_add_plugin(kdeconnect_battery SOURCES batteryplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_battery
    HEADER plugin_battery_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_BATTERY CATEGORY_NAME kdeconnect.plugin.battery
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin battery)")
target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
