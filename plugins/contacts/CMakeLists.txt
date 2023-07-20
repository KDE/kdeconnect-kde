kdeconnect_add_plugin(kdeconnect_contacts SOURCES contactsplugin.cpp)
ecm_qt_declare_logging_category(kdeconnect_contacts
    HEADER plugin_contacts_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CONTACTS CATEGORY_NAME kdeconnect.plugin.contacts
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin contacts)")

target_link_libraries(kdeconnect_contacts
    kdeconnectcore
    Qt::DBus
)
