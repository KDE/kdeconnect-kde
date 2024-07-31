kdeconnect_add_plugin(kdeconnect_mprisremote)

target_sources(kdeconnect_mprisremote PRIVATE
    mprisremoteplugin.cpp
    albumart_cache.cpp
    mprisremoteplayer.cpp
    mprisremoteplayermediaplayer2.cpp
    mprisremoteplayermediaplayer2player.cpp
)
target_link_libraries(kdeconnect_mprisremote
    kdeconnectcore
    Qt::DBus
    Qt::Core # for hash
)
