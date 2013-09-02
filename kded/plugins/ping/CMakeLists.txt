find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_ping_SRCS
    pingplugin.cpp
    ../kdeconnectplugin.cpp
)

kde4_add_plugin(kdeconnect_ping ${kdeconnect_ping_SRCS})

target_link_libraries(kdeconnect_ping
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

install(TARGETS kdeconnect_ping DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_ping.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
