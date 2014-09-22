project(KDEConnectCore)

add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

set(KDECONNECT_VERSION_MAJOR 1)
set(KDECONNECT_VERSION_MINOR 0)
set(KDECONNECT_VERSION_PATCH 1)
set(KDECONNECT_VERSION "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}")

include_directories(
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
)

add_library(kdeconnectcore SHARED ${kded_kdeconnect_SRCS})
target_link_libraries(kdeconnectcore
PUBLIC
    Qt5::Network
PRIVATE
    ${QCA2_LIBRARIES}
    Qt5::DBus
    Qt5::Gui
    KF5::KIOWidgets
    KF5::KCMUtils
    KF5::I18n
    KF5::ConfigCore
    KF5::Notifications
)

set_target_properties(kdeconnectcore PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
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
