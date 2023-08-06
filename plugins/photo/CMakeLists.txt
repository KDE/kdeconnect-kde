kdeconnect_add_plugin(kdeconnect_photo SOURCES photoplugin.cpp)
target_link_libraries(kdeconnect_photo
    kdeconnectcore
    Qt::DBus
)
