if(WIN32)
    set(kdeconnect_mpriscontrol_SRCS
        mpriscontrolplugin-win.cpp
    )
else()
   set(kdeconnect_mpriscontrol_SRCS
       mpriscontrolplugin.cpp
   )

set_source_files_properties(
    org.freedesktop.DBus.Properties.xml
    org.mpris.MediaPlayer2.Player.xml
    org.mpris.MediaPlayer2.xml
    PROPERTIES
    NO_NAMESPACE ON)

qt5_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.freedesktop.DBus.Properties.xml dbusproperties)
qt5_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.mpris.MediaPlayer2.Player.xml mprisplayer)
qt5_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.mpris.MediaPlayer2.xml mprisroot)

endif()

kdeconnect_add_plugin(kdeconnect_mpriscontrol JSON kdeconnect_mpriscontrol.json SOURCES  ${kdeconnect_mpriscontrol_SRCS})

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt5::DBus kdeconnectcore)
endif()
