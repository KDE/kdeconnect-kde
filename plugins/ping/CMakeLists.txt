kdeconnect_add_plugin(kdeconnect_ping SOURCES pingplugin.cpp)
target_link_libraries(kdeconnect_ping
    kdeconnectcore
    Qt::DBus
    KF6::I18n
)
