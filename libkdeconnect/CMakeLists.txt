set(likdeconnect_VERSION_MAJOR 1)
set(likdeconnect_VERSION_MINOR 0)
set(likdeconnect_VERSION_RELEASE 1)
set(likdeconnect_VERSION "${likdeconnect_VERSION_MAJOR}.${likdeconnect_VERSION_MINOR}.${likdeconnect_VERSION_RELEASE}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_SOURCE_DIR})

set(libkdeconnect_SRC
    dbusinterfaces.cpp
    devicesmodel.cpp
    notificationsmodel.cpp
    modeltest.cpp
    kdebugnamespace.cpp
)

set_source_files_properties(
    ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.daemon.xml
    ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.device.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/battery/org.kde.kdeconnect.device.battery.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.xml
    ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml
    PROPERTIES NO_NAMESPACE true
)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.daemon.xml daemoninterface)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/org.kde.kdeconnect.device.xml deviceinterface)
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/battery/org.kde.kdeconnect.device.battery.xml devicebatteryinterface )
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.xml devicenotificationsinterface )
qt4_add_dbus_interface(libkdeconnect_SRC ${CMAKE_BINARY_DIR}/kded/plugins/notifications/org.kde.kdeconnect.device.notifications.notification.xml notificationinterface  )

kde4_add_library(kdeconnect SHARED ${libkdeconnect_SRC})
set_target_properties(kdeconnect PROPERTIES
    VERSION ${likdeconnect_VERSION}
    SOVERSION ${likdeconnect_VERSION_MAJOR}
)

add_dependencies(kdeconnect
        org.kde.kdeconnect.daemon.xml
        org.kde.kdeconnect.device.xml
        org.kde.kdeconnect.device.battery.xml
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

install(TARGETS kdeconnect EXPORT kdeconnectLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS})
