kdeconnect_add_plugin(kdeconnect_bigscreen SOURCES bigscreenplugin.cpp)
target_link_libraries(kdeconnect_bigscreen
    kdeconnectcore
    kdeconnectinterfaces
    Qt::DBus
    KF6::I18n)
