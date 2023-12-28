kdeconnect_add_plugin(kdeconnect_battery SOURCES batteryplugin.cpp)
target_link_libraries(kdeconnect_battery
    kdeconnectcore
    Qt::DBus
    KF6::Solid
    KF6::I18n
)
