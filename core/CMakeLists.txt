add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

set(KDECONNECT_PRIVATE_DBUS_ADDR unix:tmpdir=/tmp)
if(WIN32)
    set(KDECONNECT_PRIVATE_DBUS_ADDR tcp:host=localhost,port=0)
endif()
set(KDECONNECT_PRIVATE_DBUS_NAME DBusKDEConnectOnly)
configure_file(dbushelper.h.in ${CMAKE_CURRENT_BINARY_DIR}/dbushelper.h)

option(MDNS_ENABLED "Use MDNS for device discovery" ON)

add_subdirectory(backends/lan)
add_subdirectory(backends/loopback)

option(BLUETOOTH_ENABLED "Bluetooth support for kdeconnect" ON)
if(BLUETOOTH_ENABLED)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Bluetooth)
    add_subdirectory(backends/bluetooth)
endif()

option(LOOPBACK_ENABLED "Loopback backend enabled" OFF)

add_library(kdeconnectcore)
target_sources(kdeconnectcore PRIVATE
    ${backends_kdeconnect_SRCS}

    backends/linkprovider.cpp
    backends/devicelink.cpp
    backends/pairinghandler.cpp

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
    sslhelper.cpp
    core_debug.cpp
    notificationserverinfo.cpp
    openconfig.cpp
)
ecm_qt_declare_logging_category(kdeconnectcore
    HEADER kdeconnect_debug.h
    IDENTIFIER KDECONNECT_CORE CATEGORY_NAME kdeconnect.core
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (core)")


target_include_directories(kdeconnectcore PUBLIC ${PROJECT_SOURCE_DIR})

find_package(OpenSSL REQUIRED COMPONENTS Crypto)

target_link_libraries(kdeconnectcore
PUBLIC
    Qt::Network
    KF6::CoreAddons
    KF6::KIOCore
    KF6::KIOGui
    OpenSSL::Crypto
PRIVATE
    Qt::DBus
    KF6::I18n
    KF6::ConfigCore
)

if(WIN32)
    target_link_libraries(kdeconnectcore PRIVATE ws2_32.lib) # winsock2
endif()

if (BLUETOOTH_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_BLUETOOTH)
    target_link_libraries(kdeconnectcore PRIVATE Qt6::Bluetooth)
endif()

if (LOOPBACK_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_LOOPBACK)
endif()

if (MDNS_ENABLED)
    target_compile_definitions(kdeconnectcore PRIVATE -DKDECONNECT_MDNS)
endif()

set_target_properties(kdeconnectcore PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectcore EXPORT_FILE_NAME kdeconnectcore_export.h BASE_NAME KDEConnectCore)

install(TARGETS kdeconnectcore EXPORT kdeconnectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
