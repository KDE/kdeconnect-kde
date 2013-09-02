find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_filetransfer_SRCS
    filetransferplugin.cpp
    ../kdeconnectplugin.cpp
    ../pluginloader.cpp
    ../../networkpackage.cpp
    ../../device.cpp
)

kde4_add_plugin(kdeconnect_filetransfer ${kdeconnect_filetransfer_SRCS})

target_link_libraries(kdeconnect_filetransfer
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

install(TARGETS kdeconnect_filetransfer DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_filetransfer.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
