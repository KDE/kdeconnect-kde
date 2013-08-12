find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_ping_SRCS
    pingpackageinterface.cpp
    ../packageinterface.cpp
)

kde4_add_plugin(kdeconnect_ping ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    qjson
)

install(TARGETS kdeconnect_ping DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_ping.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
