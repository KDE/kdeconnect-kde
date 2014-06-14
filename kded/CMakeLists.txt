project(KDEConnectCore)

include_directories(
    ${KDE4_KIO_LIBS}
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(plugins)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

set(kded_kdeconnect_SRCS
    ${kded_kdeconnect_SRCS}

    backends/linkprovider.cpp
    backends/devicelink.cpp

    plugins/kdeconnectplugin.cpp
    plugins/pluginloader.cpp

    networkpackage.cpp
    filetransferjob.cpp
    daemon.cpp
    device.cpp
    kdebugnamespace.cpp
)

kde4_add_library(kdeconnectcore SHARED ${kded_kdeconnect_SRCS})
target_link_libraries(kdeconnectcore
PUBLIC
    ${KDE4_KDECORE_LIBS}

PRIVATE
    ${KDE4_KIO_LIBS}
    ${QJSON_LIBRARIES}
    ${QT_QTNETWORK_LIBRARY}
    ${QCA2_LIBRARIES}
)
generate_export_header(kdeconnectcore EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectcore_export.h BASE_NAME KDEConnectCore)

kde4_add_executable(kdeconnectd kdeconnectd.cpp)
target_link_libraries(kdeconnectd kdeconnectcore ${KDE4_KDEUI_LIBS})

kde4_add_plugin(kded_kdeconnect kded.cpp)

target_link_libraries(kded_kdeconnect
    ${KDE4_KDECORE_LIBS}
)

include(../macros.cmake)

generate_and_install_dbus_interface(
    kdeconnectd
    daemon.h
    org.kde.kdeconnect.daemon.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kdeconnectd
    device.h
    org.kde.kdeconnect.device.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kded_kdeconnect
    kded.h
    org.kde.kded.kdeconnect.xml
    OPTIONS -a
)

install(TARGETS kdeconnectd DESTINATION ${LIBEXEC_INSTALL_DIR})
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)
