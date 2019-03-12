kdeconnect_add_plugin(kdeconnect_remotekeyboard JSON kdeconnect_remotekeyboard.json
    SOURCES remotekeyboardplugin.cpp)

target_link_libraries(kdeconnect_remotekeyboard
    kdeconnectcore
    Qt5::DBus
)
