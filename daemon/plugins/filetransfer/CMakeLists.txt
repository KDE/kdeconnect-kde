find_package(KDE4 REQUIRED)
include (KDE4Defaults)
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
    qjson
)

install(TARGETS kdeconnect_filetransfer DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_filetransfer.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
