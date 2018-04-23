set(kdeconnect_contacts_SRCS
    contactsplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_contacts JSON kdeconnect_contacts.json SOURCES ${kdeconnect_contacts_SRCS})

target_link_libraries(kdeconnect_contacts
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)