set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_contacts_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CONTACTS CATEGORY_NAME kdeconnect.plugin.contacts
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin contacts)")

set(kdeconnect_contacts_SRCS
    contactsplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_contacts JSON kdeconnect_contacts.json SOURCES ${kdeconnect_contacts_SRCS})

target_link_libraries(kdeconnect_contacts
    kdeconnectcore
    Qt5::DBus
)
