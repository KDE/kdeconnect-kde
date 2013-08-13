find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
    ../kdeconnectplugin.cpp
)

kde4_add_plugin(kdeconnect_telephony ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    qjson
)

install(TARGETS kdeconnect_telephony DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_telephony.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
