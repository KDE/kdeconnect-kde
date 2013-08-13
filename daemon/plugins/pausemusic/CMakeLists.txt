find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_pausemusic_SRCS
    pausemusicplugin.cpp
    ../kdeconnectplugin.cpp
)

kde4_add_plugin(kdeconnect_pausemusic ${kdeconnect_pausemusic_SRCS})

target_link_libraries(kdeconnect_pausemusic
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    qjson
)

install(TARGETS kdeconnect_pausemusic DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_pausemusic.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
