kdeconnect_add_plugin(kdeconnect_ping SOURCES pingplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_ping
    HEADER plugin_ping_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PING CATEGORY_NAME kdeconnect.plugin.ping
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin ping)")

target_link_libraries(kdeconnect_ping
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
