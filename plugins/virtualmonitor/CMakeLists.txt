kdeconnect_add_plugin(kdeconnect_virtualmonitor SOURCES virtualmonitorplugin.cpp)
ecm_qt_declare_logging_category(kdeconnect_virtualmonitor
    HEADER plugin_virtualmonitor_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_VIRTUALMONITOR CATEGORY_NAME kdeconnect.plugin.virtualmonitor
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin virtualmonitor)")

target_link_libraries(kdeconnect_virtualmonitor
    kdeconnectcore
    Qt::Core
    Qt::Multimedia
    Qt::DBus
)
