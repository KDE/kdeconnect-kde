set(kded_androidshine_SRCS

    announcers/announcer.cpp
    announcers/fakeannouncer.cpp
    announcers/avahiannouncer.cpp

    devicelinks/echodevicelink.cpp
    devicelinks/udpdevicelink.cpp
    devicelinks/devicelink.cpp

    packagereceivers/packagereceiver.cpp
    packagereceivers/pausemusicpackagereceiver.cpp
    packagereceivers/notificationpackagereceiver.cpp

    networkpackage.cpp
    daemon.cpp
    device.cpp

    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.xml
)

kde4_add_plugin(kded_androidshine ${kded_androidshine_SRCS})

target_link_libraries(kded_androidshine
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kdnssd
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QJSON_LIBRARY}
)

qt4_generate_dbus_interface(
    daemon.h
    org.kde.kdeconnect.xml
    OPTIONS -a
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kdeconnect.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(TARGETS kded_androidshine DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES androidshine.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES androidshine.notifyrc DESTINATION ${DATA_INSTALL_DIR}/androidshine)
