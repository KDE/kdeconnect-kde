kdeconnect_add_plugin(kdeconnect_mprisremote JSON kdeconnect_mprisremote.json SOURCES mprisremoteplugin.cpp)

target_link_libraries(kdeconnect_mprisremote
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)
