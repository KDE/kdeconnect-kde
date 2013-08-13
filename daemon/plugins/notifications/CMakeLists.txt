find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_notifications_SRCS
    notificationsplugin.cpp
    ../kdeconnectplugin.cpp
)

kde4_add_plugin(kdeconnect_notifications ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    qjson
)

install(TARGETS kdeconnect_notifications DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_notifications.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
