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

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_mpris_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MPRIS CATEGORY_NAME kdeconnect.plugin.mpris
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mpris)")

kdeconnect_add_plugin(kdeconnect_mpriscontrol JSON kdeconnect_mpriscontrol.json SOURCES  ${kdeconnect_mpriscontrol_SRCS} ${debug_file_SRCS})

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore windowsapp)
   target_compile_features(kdeconnect_mpriscontrol PUBLIC cxx_std_17)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt5::DBus kdeconnectcore)
endif()
