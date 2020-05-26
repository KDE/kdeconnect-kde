set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_remotecommands_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTECOMMANDS CATEGORY_NAME kdeconnect.plugin.remotecommands
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotecommands)")

kdeconnect_add_plugin(kdeconnect_remotecommands JSON kdeconnect_remotecommands.json
    SOURCES remotecommandsplugin.cpp ${debug_file_SRCS}
)


target_link_libraries(kdeconnect_remotecommands
    kdeconnectcore
    Qt5::DBus
    )
