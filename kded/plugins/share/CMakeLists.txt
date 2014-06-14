set(kdeconnect_share_SRCS
    shareplugin.cpp
    autoclosingqfile.cpp
)

kde4_add_plugin(kdeconnect_share ${kdeconnect_share_SRCS})

target_link_libraries(kdeconnect_share kdeconnectcore ${KDE4_KFILE_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kdeconnect_share DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_share.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


#######################################
# Config

set( kdeconnect_share_config_SRCS share_config.cpp )
kde4_add_ui_files( kdeconnect_share_config_SRCS share_config.ui )

kde4_add_plugin( kdeconnect_share_config ${kdeconnect_share_config_SRCS} )
target_link_libraries( kdeconnect_share_config
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
)

install(TARGETS kdeconnect_share_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_share_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
