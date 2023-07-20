kdeconnect_add_plugin(kdeconnect_remotecommands SOURCES remotecommandsplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_remotecommands
    HEADER plugin_remotecommands_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTECOMMANDS CATEGORY_NAME kdeconnect.plugin.remotecommands
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotecommands)")
target_link_libraries(kdeconnect_remotecommands
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
)
