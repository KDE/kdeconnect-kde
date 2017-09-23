find_package(KF5 REQUIRED COMPONENTS KCMUtils)
find_program(PACTL pactl)

kdeconnect_add_plugin(kdeconnect_pausemusic JSON kdeconnect_pausemusic.json SOURCES pausemusicplugin.cpp)

target_link_libraries(kdeconnect_pausemusic
    kdeconnectcore
    Qt5::Core
    Qt5::DBus
)

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
