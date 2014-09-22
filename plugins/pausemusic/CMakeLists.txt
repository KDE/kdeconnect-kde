set(kdeconnect_pausemusic_SRCS
    pausemusicplugin.cpp
)

add_library(kdeconnect_pausemusic MODULE ${kdeconnect_pausemusic_SRCS})

target_link_libraries(kdeconnect_pausemusic
    Qt5::Core
    Qt5::DBus
    KF5::ConfigCore
    KF5::Service
    kdeconnectcore
)

install(TARGETS kdeconnect_pausemusic DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_pausemusic.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


#######################################
# Config

set( kdeconnect_pausemusic_config_SRCS pausemusic_config.cpp )
ki18n_wrap_ui( kdeconnect_pausemusic_config_SRCS pausemusic_config.ui )

add_library(kdeconnect_pausemusic_config MODULE ${kdeconnect_pausemusic_config_SRCS} )
target_link_libraries( kdeconnect_pausemusic_config
    Qt5::Core
    KF5::KDELibs4Support
)

install( TARGETS kdeconnect_pausemusic_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kdeconnect_pausemusic_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
