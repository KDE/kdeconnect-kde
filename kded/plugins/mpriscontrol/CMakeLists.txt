find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${QCA2_INCLUDE_DIR}
)

include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})

set(kdeconnect_mpriscontrol_SRCS
    mpriscontrolplugin.cpp
    ../kdeconnectplugin.cpp
    ../pluginloader.cpp
    ../../networkpackage.cpp
    ../../device.cpp
)

qt4_add_dbus_interface(
   kdeconnect_mpriscontrol_SRCS
   mprisdbusinterface.xml
   mprisdbusinterface
)

qt4_add_dbus_interface(
   kdeconnect_mpriscontrol_SRCS
   propertiesInterface.xml
   propertiesdbusinterface
)

kde4_add_plugin(kdeconnect_mpriscontrol ${kdeconnect_mpriscontrol_SRCS})

target_link_libraries(kdeconnect_mpriscontrol
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QJSON_LIBRARIES}
    ${QCA2_LIBRARIES}
)

install(TARGETS kdeconnect_mpriscontrol DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_mpriscontrol.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

