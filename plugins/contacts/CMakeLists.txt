kdeconnect_add_plugin(kdeconnect_contacts SOURCES contactsplugin.cpp)
target_link_libraries(kdeconnect_contacts
    kdeconnectcore
    Qt::DBus
)
