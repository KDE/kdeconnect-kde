set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
)

kde4_add_plugin(kdeconnect_clipboard ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore ${QT_QTGUI_LIBRARY})

install(TARGETS kdeconnect_clipboard DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_clipboard.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
