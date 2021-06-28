set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_mprisremote_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MPRISREMOTE CATEGORY_NAME kdeconnect.plugin.mprisremote
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mprisremote)")

kdeconnect_add_plugin(kdeconnect_mprisremote SOURCES mprisremoteplugin.cpp mprisremoteplayer.cpp mprisremoteplayermediaplayer2.cpp mprisremoteplayermediaplayer2player.cpp ${debug_file_SRCS})

target_link_libraries(kdeconnect_mprisremote
    kdeconnectcore
    Qt5::DBus
)
