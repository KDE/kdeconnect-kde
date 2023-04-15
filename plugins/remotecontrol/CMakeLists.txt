set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_remotecontrol_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTECONTROL CATEGORY_NAME kdeconnect.plugin.remotecontrol
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotecontrol)")

kdeconnect_add_plugin(kdeconnect_remotecontrol SOURCES remotecontrolplugin.cpp ${debug_file_SRCS})

target_link_libraries(kdeconnect_remotecontrol
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
