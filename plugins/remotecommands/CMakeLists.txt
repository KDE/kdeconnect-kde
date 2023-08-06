kdeconnect_add_plugin(kdeconnect_remotecommands SOURCES remotecommandsplugin.cpp)
target_link_libraries(kdeconnect_remotecommands
    kdeconnectcore
    Qt::DBus
)
