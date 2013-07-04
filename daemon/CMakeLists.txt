set(kded_kdeconnect_SRCS

    announcers/announcer.cpp
    announcers/fakeannouncer.cpp
    announcers/avahiannouncer.cpp

    devicelinks/echodevicelink.cpp
    devicelinks/udpdevicelink.cpp
    devicelinks/devicelink.cpp

    packagereceivers/packagereceiver.cpp
    packagereceivers/pingpackagereceiver.cpp
    packagereceivers/notificationpackagereceiver.cpp
    packagereceivers/pausemusicpackagereceiver.cpp

    networkpackage.cpp
    daemon.cpp
    device.cpp

    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.xml
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.device.xml
)

kde4_add_plugin(kded_kdeconnect ${kded_kdeconnect_SRCS})

target_link_libraries(kded_kdeconnect
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kdnssd
    qjson
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QJSON_LIBRARY}
)

qt4_generate_dbus_interface(
    daemon.h
    org.kde.kdeconnect.xml
    OPTIONS -a
)

qt4_generate_dbus_interface(
    device.h
    org.kde.kdeconnect.device.xml
    OPTIONS -a
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.device.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)
