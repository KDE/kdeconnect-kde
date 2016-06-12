kdeconnect_add_plugin(kdeconnect_remotecommands JSON kdeconnect_remotecommands.json
    SOURCES remotecommandsplugin.cpp
)

target_link_libraries(kdeconnect_remotecommands
    kdeconnectcore
    Qt5::DBus
    KF5::I18n)
