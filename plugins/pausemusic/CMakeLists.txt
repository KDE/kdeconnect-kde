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

install(FILES "kdeconnect_pausemusic_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
