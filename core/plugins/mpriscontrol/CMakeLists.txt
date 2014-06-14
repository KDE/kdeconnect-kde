set(kdeconnect_mpriscontrol_SRCS
    mpriscontrolplugin.cpp
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

target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore)

install(TARGETS kdeconnect_mpriscontrol DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_mpriscontrol.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

