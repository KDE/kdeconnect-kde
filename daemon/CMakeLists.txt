set(kded_kdeconnect_SRCS

    linkproviders/linkprovider.cpp
    linkproviders/loopbacklinkprovider.cpp
    linkproviders/lanlinkprovider.cpp

    devicelinks/devicelink.cpp
    devicelinks/loopbackdevicelink.cpp
    devicelinks/landevicelink.cpp

    plugins/kdeconnectplugin.cpp
    plugins/pluginloader.cpp

    networkpackage.cpp
    daemon.cpp
    device.cpp
)

add_subdirectory(plugins)

kde4_add_plugin(kded_kdeconnect ${kded_kdeconnect_SRCS})

target_link_libraries(kded_kdeconnect
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    #kdnssd
    qjson
    ${QT_QTNETWORK_LIBRARY}
)

include(../cmakemacros.txt)

generate_and_install_dbus_interface(
    kded_kdeconnect
    daemon.h
    org.kde.kdeconnect.daemon.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kded_kdeconnect
    device.h
    org.kde.kdeconnect.device.xml
    OPTIONS -a
)

install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)
