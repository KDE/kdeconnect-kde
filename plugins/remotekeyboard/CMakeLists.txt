kdeconnect_add_plugin(kdeconnect_remotekeyboard SOURCES remotekeyboardplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_remotekeyboard
    HEADER plugin_remotekeyboard_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTEKEYBOARD CATEGORY_NAME kdeconnect.plugin.remotekeyboard
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotekeyboard)")

target_link_libraries(kdeconnect_remotekeyboard
    kdeconnectcore
    Qt::DBus
)
