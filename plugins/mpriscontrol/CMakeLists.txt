if(WIN32)
    set(kdeconnect_mpriscontrol_SRCS
        mpriscontrolplugin-win.cpp
    )
else()
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
endif()

kdeconnect_add_plugin(kdeconnect_mpriscontrol JSON kdeconnect_mpriscontrol.json SOURCES  ${kdeconnect_mpriscontrol_SRCS})

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt5::DBus kdeconnectcore)
endif()