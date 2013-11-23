find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include(KDE4Defaults)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_share_SRCS
    shareplugin.cpp
    autoclosingqfile.cpp
    ../kdeconnectplugin.cpp
    ../pluginloader.cpp
    ../../filetransferjob.cpp
    ../../networkpackage.cpp
    ../../device.cpp
    ../../kdebugnamespace.cpp
)

kde4_add_plugin(kdeconnect_share ${kdeconnect_share_SRCS})

target_link_libraries(kdeconnect_share
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

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
