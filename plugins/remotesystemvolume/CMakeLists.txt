kdeconnect_add_plugin(kdeconnect_remotesystemvolume SOURCES remotesystemvolumeplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_remotesystemvolume
    HEADER plugin_remotesystemvolume_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTESYSTEMVOLUME CATEGORY_NAME kdeconnect.plugin.remotesystemvolume
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotesystemvolume)")
target_link_libraries(kdeconnect_remotesystemvolume
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
