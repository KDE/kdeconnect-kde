kdeconnect_add_plugin(kdeconnect_mprisremote JSON kdeconnect_mprisremote.json SOURCES mprisremoteplugin.cpp mprisremoteplayer.cpp)

target_link_libraries(kdeconnect_mprisremote
    kdeconnectcore
    Qt5::DBus
)
