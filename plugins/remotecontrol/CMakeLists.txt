kdeconnect_add_plugin(kdeconnect_remotecontrol JSON kdeconnect_remotecontrol.json SOURCES remotecontrolplugin.cpp)

target_link_libraries(kdeconnect_remotecontrol
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)
