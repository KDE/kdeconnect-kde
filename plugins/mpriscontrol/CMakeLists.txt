set(kdeconnect_mpriscontrol_SRCS
    mpriscontrolplugin.cpp
)

qt5_add_dbus_interface(
   kdeconnect_mpriscontrol_SRCS
   mprisdbusinterface.xml
   mprisdbusinterface
)

qt5_add_dbus_interface(
   kdeconnect_mpriscontrol_SRCS
   propertiesInterface.xml
   propertiesdbusinterface
)

add_library(kdeconnect_mpriscontrol MODULE ${kdeconnect_mpriscontrol_SRCS})

target_link_libraries(kdeconnect_mpriscontrol Qt5::DBus KF5::Service kdeconnectcore)

install(TARGETS kdeconnect_mpriscontrol DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_mpriscontrol.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

