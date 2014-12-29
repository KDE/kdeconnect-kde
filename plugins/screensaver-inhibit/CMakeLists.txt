set(kdeconnect_screensaver_inhibit_SRCS
    screensaverinhibitplugin.cpp
)

kde4_add_plugin(kdeconnect_screensaver_inhibit ${kdeconnect_screensaver_inhibit_SRCS})

target_link_libraries(kdeconnect_screensaver_inhibit kdeconnectcore ${KDE4_KDEUI_LIBS})

install(TARGETS kdeconnect_screensaver_inhibit DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_screensaver_inhibit.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
