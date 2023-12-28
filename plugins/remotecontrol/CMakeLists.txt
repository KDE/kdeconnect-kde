kdeconnect_add_plugin(kdeconnect_remotecontrol SOURCES remotecontrolplugin.cpp)
target_link_libraries(kdeconnect_remotecontrol
    kdeconnectcore
    Qt::DBus
    KF6::I18n
)
