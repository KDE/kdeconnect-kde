kdeconnect_add_plugin(kdeconnect_photo SOURCES photoplugin.cpp)

ecm_qt_declare_logging_category(kdeconnect_photo
    HEADER plugin_photo_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PHOTO CATEGORY_NAME kdeconnect.plugin.photo
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin photo)")
target_link_libraries(kdeconnect_photo
    kdeconnectcore
    Qt::DBus
)
