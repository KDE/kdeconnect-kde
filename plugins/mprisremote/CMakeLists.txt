kdeconnect_add_plugin(kdeconnect_mprisremote)

target_sources(kdeconnect_mprisremote PRIVATE
    mprisremoteplugin.cpp
    mprisremoteplayer.cpp
    mprisremoteplayermediaplayer2.cpp
    mprisremoteplayermediaplayer2player.cpp
)
ecm_qt_declare_logging_category(kdeconnect_mprisremote
    HEADER plugin_mprisremote_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MPRISREMOTE CATEGORY_NAME kdeconnect.plugin.mprisremote
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mprisremote)")
target_link_libraries(kdeconnect_mprisremote
    kdeconnectcore
    Qt::DBus
)
