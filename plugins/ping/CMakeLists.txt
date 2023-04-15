set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_ping_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PING CATEGORY_NAME kdeconnect.plugin.ping
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin ping)")

set(kdeconnect_ping_SRCS
    pingplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_ping SOURCES ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
