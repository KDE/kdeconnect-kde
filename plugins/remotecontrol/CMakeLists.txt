kdeconnect_add_plugin(kdeconnect_remotecontrol SOURCES remotecontrolplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_remotecontrol
    HEADER plugin_remotecontrol_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTECONTROL CATEGORY_NAME kdeconnect.plugin.remotecontrol
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotecontrol)")
target_link_libraries(kdeconnect_remotecontrol
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
