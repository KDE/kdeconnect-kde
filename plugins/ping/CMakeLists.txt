set(kdeconnect_ping_SRCS
    pingplugin.cpp
)

kde4_add_plugin(kdeconnect_ping ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping kdeconnectcore ${KDE4_KDEUI_LIBS})

install(TARGETS kdeconnect_ping DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_ping.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
