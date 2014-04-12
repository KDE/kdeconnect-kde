set(KDECONNECT_VERSION_MAJOR 1)
set(KDECONNECT_VERSION_MINOR 0)
set(KDECONNECT_VERSION_PATCH 1)
set(KDECONNECT_VERSION "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_SOURCE_DIR})

add_definitions(-DKDECONNECT_LIBRARY)

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
    modeltest.cpp
    kdebugnamespace.cpp
)

set(libkdeconnect_public_HEADERS
    KDEConnect/DevicesModel
    KDEConnect/NotificationsModel
)

set(libkdeconnect_HEADERS
    devicesmodel.h
    notificationsmodel.h
    dbusinterfaces.h
    kdeconnect_export.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/daemoninterface.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/deviceinterface.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/devicebatteryinterface.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/devicesftpinterface.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/devicenotificationsinterface.h
    ${CMAKE_BINARY_DIR}/libkdeconnect/notificationinterface.h
)

set_source_files_properties(
    ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.daemon.xml
    ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.device.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/battery/org.kde.kdeconnect.device.battery.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/sftp/org.kde.kdeconnect.device.sftp.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml
    PROPERTIES NO_NAMESPACE true
)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.daemon.xml daemoninterface)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.device.xml deviceinterface)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/battery/org.kde.kdeconnect.device.battery.xml devicebatteryinterface )
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/sftp/org.kde.kdeconnect.device.sftp.xml devicesftpinterface )
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.xml devicenotificationsinterface )
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml notificationinterface  )

kde4_add_library(kdeconnect SHARED ${libkdeconnect_SRC})
set_target_properties(kdeconnect PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

add_dependencies(kdeconnect
        org.kde.kdeconnect.daemon.xml
        org.kde.kdeconnect.device.xml
        org.kde.kdeconnect.device.battery.xml
        org.kde.kdeconnect.device.sftp.xml
        org.kde.kdeconnect.device.notifications.xml
        org.kde.kdeconnect.device.notifications.notification.xml
)

target_link_libraries(kdeconnect
    ${KDE4_KDEUI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${QJSON_LIBRARIES}
)

configure_file(KDEConnectConfig.cmake.in ${CMAKE_BINARY_DIR}/libkdeconnect/KDEConnectConfig.cmake @ONLY)

macro_write_basic_cmake_version_file(${CMAKE_BINARY_DIR}/libkdeconnect/KDEConnectConfigVersion.cmake
        ${KDECONNECT_VERSION_MAJOR} ${KDECONNECT_VERSION_MINOR} ${KDECONNECT_VERSION_PATCH})
        
install(TARGETS kdeconnect EXPORT kdeconnectLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS})

#Install header files
install(FILES ${libkdeconnect_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/kdeconnect COMPONENT Devel)
install(FILES ${libkdeconnect_public_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/KDEConnect COMPONENT Devel)
install(FILES ${CMAKE_BINARY_DIR}/libkdeconnect/KDEConnectConfig.cmake
              ${CMAKE_BINARY_DIR}/libkdeconnect/KDEConnectConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/KDEConnect)
