add_subdirectory(plugins)

find_package (QJSON REQUIRED)
find_package (QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

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

kde4_add_plugin(kded_kdeconnect ${kded_kdeconnect_SRCS})

target_link_libraries(kded_kdeconnect
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

include(../macros.cmake)

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
