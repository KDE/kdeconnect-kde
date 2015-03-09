project(KDEConnectInterfaces)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_SOURCE_DIR})

add_definitions(-DKDECONNECT_LIBRARY)

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
#    modeltest.cpp
)

set(libkdeconnect_public_HEADERS
    KDEConnect/DevicesModel
    KDEConnect/NotificationsModel
)

set(libkdeconnect_HEADERS
    devicesmodel.h
    notificationsmodel.h
    dbusinterfaces.h
    ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h
    ${CMAKE_BINARY_DIR}/interfaces/daemoninterface.h
    ${CMAKE_BINARY_DIR}/interfaces/deviceinterface.h
    ${CMAKE_BINARY_DIR}/interfaces/devicebatteryinterface.h
    ${CMAKE_BINARY_DIR}/interfaces/devicesftpinterface.h
    ${CMAKE_BINARY_DIR}/interfaces/devicenotificationsinterface.h
    ${CMAKE_BINARY_DIR}/interfaces/notificationinterface.h
)

set_source_files_properties(
    ${CMAKE_BINARY_DIR}/core/org.kde.kdeconnect.daemon.xml
    ${CMAKE_BINARY_DIR}/core/org.kde.kdeconnect.device.xml
    ${CMAKE_BINARY_DIR}/plugins/battery/org.kde.kdeconnect.device.battery.xml
    ${CMAKE_BINARY_DIR}/plugins/sftp/org.kde.kdeconnect.device.sftp.xml
    ${CMAKE_BINARY_DIR}/plugins/notifications/org.kde.kdeconnect.device.notifications.xml
    ${CMAKE_BINARY_DIR}/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml
    PROPERTIES NO_NAMESPACE true
)
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/core/org.kde.kdeconnect.daemon.xml daemoninterface)
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/core/org.kde.kdeconnect.device.xml deviceinterface)
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/plugins/battery/org.kde.kdeconnect.device.battery.xml devicebatteryinterface )
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/plugins/sftp/org.kde.kdeconnect.device.sftp.xml devicesftpinterface )
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/plugins/notifications/org.kde.kdeconnect.device.notifications.xml devicenotificationsinterface )
qt5_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml notificationinterface  )

add_library(kdeconnectinterfaces SHARED ${libkdeconnect_SRC})
set_target_properties(kdeconnectinterfaces PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectinterfaces EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_export.h BASE_NAME KDEConnectInterfaces)

add_dependencies(kdeconnectinterfaces
        org.kde.kdeconnect.daemon.xml
        org.kde.kdeconnect.device.xml
        org.kde.kdeconnect.device.battery.xml
        org.kde.kdeconnect.device.sftp.xml
        org.kde.kdeconnect.device.notifications.xml
        org.kde.kdeconnect.device.notifications.notification.xml
)

target_link_libraries(kdeconnectinterfaces
LINK_PUBLIC
    Qt5::Gui
    Qt5::DBus
LINK_PRIVATE
    KF5::ConfigCore
)

configure_file(KDEConnectConfig.cmake.in ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfig.cmake @ONLY)

ecm_setup_version( "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}" VARIABLE_PREFIX KDECONNECTINTERFACES
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kdeconnectinterfaces_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KDEConnectConfigVersion.cmake"
                        SOVERSION ${KDECONNECT_VERSION_MAJOR})
        
install(TARGETS kdeconnectinterfaces EXPORT kdeconnectLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS})

#Install header files
install(FILES ${libkdeconnect_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/kdeconnect COMPONENT Devel)
install(FILES ${libkdeconnect_public_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KDEConnect COMPONENT Devel)
install(FILES ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfig.cmake
              ${CMAKE_BINARY_DIR}/interfaces/KDEConnectConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/KDEConnect)
