kdeconnect_add_plugin(kdeconnect_bigscreen SOURCES bigscreenplugin.cpp)

target_link_libraries(kdeconnect_bigscreen
    kdeconnectcore
    kdeconnectinterfaces
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n)
