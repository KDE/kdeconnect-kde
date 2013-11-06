find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include(KDE4Defaults)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${CMAKE_SOURCE_DIR}
)

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
    ../kdeconnectplugin.cpp
    ../pluginloader.cpp
    ../../networkpackage.cpp
    ../../filetransferjob.cpp
    ../../device.cpp
    ../../kdebugnamespace.cpp
)

kde4_add_plugin(kdeconnect_notifications ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

install(TARGETS kdeconnect_notifications DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_notifications.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

include(../../../macros.cmake)
generate_and_install_dbus_interface(
    kdeconnect_notifications
    notificationsdbusinterface.h
    org.kde.kdeconnect.device.notifications.xml
    OPTIONS -a
)
generate_and_install_dbus_interface(
    kdeconnect_notifications
    notification.h
    org.kde.kdeconnect.device.notifications.notification.xml
    OPTIONS -a
)
