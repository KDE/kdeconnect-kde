project(KDEConnectCore)

include_directories(
    ${KDE4_KIO_LIBS}
    ${QJSON_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${QCA2_INCLUDE_DIR}
)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

set(kded_kdeconnect_SRCS
    ${kded_kdeconnect_SRCS}

    backends/linkprovider.cpp
    backends/devicelink.cpp

    kdeconnectplugin.cpp
    pluginloader.cpp

    networkpackage.cpp
    filetransferjob.cpp
    daemon.cpp
    device.cpp
    kdebugnamespace.cpp
)

kde4_add_library(kdeconnectcore SHARED ${kded_kdeconnect_SRCS})
target_link_libraries(kdeconnectcore
LINK_PUBLIC
    ${KDE4_KDECORE_LIBS}
LINK_PRIVATE
    ${KDE4_KIO_LIBS}
    ${QJSON_LIBRARIES}
    ${QT_QTNETWORK_LIBRARY}
    ${QCA2_LIBRARIES}
)
target_include_directories(kdeconnectcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(kdeconnectcore EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectcore_export.h BASE_NAME KDEConnectCore)

include(macros)

generate_and_install_dbus_interface(
    kdeconnectcore
    ../core/daemon.h
    org.kde.kdeconnect.daemon.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kdeconnectcore
    ../core/device.h
    org.kde.kdeconnect.device.xml
    OPTIONS -a
)

install(TARGETS kdeconnectcore EXPORT kdeconnectLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS})
