set(kdeconnect_pausemusic_SRCS
    pausemusicplugin.cpp
)

kde4_add_plugin(kdeconnect_pausemusic ${kdeconnect_pausemusic_SRCS})

target_link_libraries(kdeconnect_pausemusic
    kdeconnectcore
)

install(TARGETS kdeconnect_pausemusic DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_pausemusic.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


#######################################
# Config

set( kdeconnect_pausemusic_config_SRCS pausemusic_config.cpp )
kde4_add_ui_files( kdeconnect_pausemusic_config_SRCS pausemusic_config.ui )

kde4_add_plugin( kdeconnect_pausemusic_config ${kdeconnect_pausemusic_config_SRCS} )
target_link_libraries( kdeconnect_pausemusic_config
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
)

install( TARGETS kdeconnect_pausemusic_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kdeconnect_pausemusic_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
