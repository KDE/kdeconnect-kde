kdeconnect_add_plugin(kdeconnect_remotekeyboard SOURCES remotekeyboardplugin.cpp)
target_link_libraries(kdeconnect_remotekeyboard
    kdeconnectcore
    Qt::DBus
)
