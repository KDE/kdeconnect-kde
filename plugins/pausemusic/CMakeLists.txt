if(WIN32)
    set(kdeconnect_pausemusic_SRCS
        pausemusicplugin-win.cpp
    )
else()
    set(kdeconnect_pausemusic_SRCS
        pausemusicplugin.cpp
    )
endif()

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_pausemusic_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PAUSEMUSIC CATEGORY_NAME kdeconnect.plugin.pausemusic
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin pausemusic)")

qt5_add_dbus_interface(kdeconnect_pausemusic_SRCS org.mpris.MediaPlayer2.Player.xml mprisplayer)

kdeconnect_add_plugin(kdeconnect_pausemusic JSON kdeconnect_pausemusic.json SOURCES ${kdeconnect_pausemusic_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_pausemusic
    kdeconnectcore
    Qt5::Core
    Qt5::DBus
)
if (NOT WIN32)
    target_link_libraries(kdeconnect_pausemusic
        KF5::PulseAudioQt
    )
endif()

#######################################
# Config

set( kdeconnect_pausemusic_config_SRCS pausemusic_config.cpp )
ki18n_wrap_ui( kdeconnect_pausemusic_config_SRCS pausemusic_config.ui )

add_library(kdeconnect_pausemusic_config MODULE ${kdeconnect_pausemusic_config_SRCS} )
target_link_libraries( kdeconnect_pausemusic_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF5::I18n
    KF5::KCMUtils
)

install( TARGETS kdeconnect_pausemusic_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kdeconnect_pausemusic_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
