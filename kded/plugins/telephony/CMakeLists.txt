set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
)

kde4_add_plugin(kdeconnect_telephony ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony kdeconnectcore ${KDE4_KDEUI_LIBS})

install(TARGETS kdeconnect_telephony DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_telephony.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
