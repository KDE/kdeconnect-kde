set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
)

add_library(kdeconnect_clipboard MODULE ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore Qt5::Gui)

install(TARGETS kdeconnect_clipboard DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_clipboard.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
