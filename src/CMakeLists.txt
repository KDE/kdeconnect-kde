set(kded_androidshine_SRCS
    fakeannouncer.cpp
    echodevicelink.cpp
    udpdevicelink.cpp
    avahiannouncer.cpp
    pausemusicpackagereceiver.cpp
    notificationpackagereceiver.cpp
    networkpackage.cpp
    daemon.cpp
    devicelink.cpp
    announcer.cpp
    packagereceiver.cpp
)

kde4_add_plugin(kded_androidshine ${kded_androidshine_SRCS})

target_link_libraries(kded_androidshine
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kdnssd
    ${QT_QTNETWORK_LIBRARY}
)

install(TARGETS kded_androidshine DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES androidshine.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES androidshine.notifyrc DESTINATION ${DATA_INSTALL_DIR}/androidshine)
