kdeconnect_add_plugin(kdeconnect_pausemusic)

if(WIN32)
    target_sources(kdeconnect_pausemusic PRIVATE pausemusicplugin-win.cpp)
else()
    target_sources(kdeconnect_pausemusic PRIVATE pausemusicplugin.cpp)
endif()

target_link_libraries(kdeconnect_pausemusic
    kdeconnectcore
    kdeconnectinterfaces
    Qt::Core
    Qt::DBus
)
if (WIN32)
    target_link_libraries(kdeconnect_pausemusic windowsapp)
else()
    target_link_libraries(kdeconnect_pausemusic KF6::PulseAudioQt)
endif()

#######################################
# Config

kdeconnect_add_kcm(kdeconnect_pausemusic_config SOURCES pausemusic_config.cpp)

ki18n_wrap_ui(kdeconnect_pausemusic_config pausemusic_config.ui)
target_link_libraries(kdeconnect_pausemusic_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF6::I18n
    KF6::KCMUtils
)
