set(kded_kdeconnect_SRCS

    linkproviders/linkprovider.cpp
    linkproviders/loopbacklinkprovider.cpp
    linkproviders/avahitcplinkprovider.cpp
    linkproviders/broadcasttcplinkprovider.cpp

    devicelinks/devicelink.cpp
    devicelinks/echodevicelink.cpp
    devicelinks/tcpdevicelink.cpp

    packageinterfaces/packageinterface.cpp
    packageinterfaces/pingpackageinterface.cpp
    packageinterfaces/notificationpackageinterface.cpp
    packageinterfaces/pausemusicpackageinterface.cpp
    packageinterfaces/clipboardpackageinterface.cpp
    packageinterfaces/batterypackageinterface.cpp
    packageinterfaces/mpriscontrolpackageinterface.cpp
    packageinterfaces/devicebatteryinformation_p.cpp

    networkpackage.cpp
    daemon.cpp
    device.cpp
)


qt4_add_dbus_interface(
   kded_kdeconnect_SRCS
   packageinterfaces/mprisdbusinterface.xml
   mprisdbusinterface
)

qt4_add_dbus_interface(
   kded_kdeconnect_SRCS
   packageinterfaces/propertiesInterface.xml
   propertiesdbusinterface
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

macro (generate_and_install_dbus_interface main_project_target header_file output_xml_file) #OPTIONS qdbus_options
    QT4_EXTRACT_OPTIONS(
        extra_files_ignore
        qdbus_options
        ${ARGN}
    )
    qt4_generate_dbus_interface(
        ${header_file}
        ${output_xml_file}
        OPTIONS ${qdbus_options}
    )
    add_custom_target(
        ${output_xml_file}
        SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${output_xml_file}
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
    )
    add_dependencies(
        ${main_project_target}
        ${output_xml_file}
    )
endmacro (generate_and_install_dbus_interface)

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

generate_and_install_dbus_interface(
    kded_kdeconnect
    packageinterfaces/devicebatteryinformation_p.h
    org.kde.kdeconnect.device.battery.xml
    OPTIONS -a
)

generate_and_install_dbus_interface(
    kded_kdeconnect
    packageinterfaces/batterypackageinterface.h
    org.kde.kdeconnect.battery.xml
    OPTIONS -a
)

install(TARGETS kded_kdeconnect DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kdeconnect.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES kdeconnect.notifyrc DESTINATION ${DATA_INSTALL_DIR}/kdeconnect)
