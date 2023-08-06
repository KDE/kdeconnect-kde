kdeconnect_add_plugin(kdeconnect_battery SOURCES batteryplugin.cpp)
target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
