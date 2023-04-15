add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

set(debug_file_SRCS)

ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnect_debug.h
    IDENTIFIER KDECONNECT_CORE CATEGORY_NAME kdeconnect.core
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (core)")

set(KDECONNECT_PRIVATE_DBUS_ADDR unix:tmpdir=/tmp)
if(WIN32)
    set(KDECONNECT_PRIVATE_DBUS_ADDR tcp:host=localhost,port=0)
endif()
set(KDECONNECT_PRIVATE_DBUS_NAME DBusKDEConnectOnly)
configure_file(dbushelper.h.in ${CMAKE_CURRENT_BINARY_DIR}/dbushelper.h)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

option(BLUETOOTH_ENABLED "Bluetooth support for kdeconnect" OFF)
if(BLUETOOTH_ENABLED)
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Bluetooth)
    add_subdirectory(backends/bluetooth)
endif()

option(LOOPBACK_ENABLED "Loopback backend enabled" OFF)

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
    networkpacket.cpp
    filetransferjob.cpp
    compositefiletransferjob.cpp
    daemon.cpp
    device.cpp
    core_debug.cpp
    notificationserverinfo.cpp
    openconfig.cpp
    ${debug_file_SRCS}
)

add_library(kdeconnectcore ${kdeconnectcore_SRCS})

target_include_directories(kdeconnectcore PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(kdeconnectcore
PUBLIC
    Qt${QT_MAJOR_VERSION}::Network
    KF${QT_MAJOR_VERSION}::CoreAddons
    ${Qca_LIBRARY}
PRIVATE
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::ConfigCore
)

if(${KF5KIO_FOUND})
    target_link_libraries(kdeconnectcore PUBLIC KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::KIOGui)
endif()

if (BLUETOOTH_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_BLUETOOTH)
    target_link_libraries(kdeconnectcore PRIVATE Qt5::Bluetooth)
endif()

if (LOOPBACK_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_LOOPBACK)
endif()

set_target_properties(kdeconnectcore PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectcore EXPORT_FILE_NAME kdeconnectcore_export.h BASE_NAME KDEConnectCore)

install(TARGETS kdeconnectcore EXPORT kdeconnectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
