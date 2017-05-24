project(KDEConnectCore)

add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

option(BLUETOOTH_ENABLED "Bluetooth support for kdeconnect" OFF)
if(BLUETOOTH_ENABLED)
    find_package(Qt5 REQUIRED COMPONENTS Bluetooth)
    add_subdirectory(backends/bluetooth)
endif()

find_package(KF5Notifications 5.9 REQUIRED)

set(kdeconnectcore_SRCS
    ${backends_kdeconnect_SRCS}

    backends/linkprovider.cpp
    backends/devicelink.cpp
    backends/pairinghandler.cpp
    backends/devicelinereader.cpp

    kdeconnectplugin.cpp
    kdeconnectpluginconfig.cpp
    pluginloader.cpp

    kdeconnectconfig.cpp
    dbushelper.cpp
    networkpackage.cpp
    filetransferjob.cpp
    daemon.cpp
    device.cpp
    core_debug.cpp
)

add_library(kdeconnectcore ${kdeconnectcore_SRCS})
target_link_libraries(kdeconnectcore
PUBLIC
    Qt5::Network
    KF5::CoreAddons
    qca-qt5
PRIVATE
    Qt5::DBus
    Qt5::Gui
    KF5::I18n
    KF5::ConfigCore
)

if (BLUETOOTH_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_BLUETOOTH)
    target_link_libraries(kdeconnectcore PRIVATE Qt5::Bluetooth)
endif()

set_target_properties(kdeconnectcore PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

target_include_directories(kdeconnectcore PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(kdeconnectcore EXPORT_FILE_NAME kdeconnectcore_export.h BASE_NAME KDEConnectCore)

install(TARGETS kdeconnectcore EXPORT kdeconnectLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
