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

qt_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.freedesktop.DBus.Properties.xml dbusproperties)
qt_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.mpris.MediaPlayer2.Player.xml mprisplayer)
qt_add_dbus_interface(kdeconnect_mpriscontrol_SRCS org.mpris.MediaPlayer2.xml mprisroot)

endif()

kdeconnect_add_plugin(kdeconnect_mpriscontrol SOURCES  ${kdeconnect_mpriscontrol_SRCS})

ecm_qt_declare_logging_category(kdeconnect_mpriscontrol
    HEADER plugin_mpris_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MPRIS CATEGORY_NAME kdeconnect.plugin.mpris
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mpris)")
if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore windowsapp)
   target_compile_features(kdeconnect_mpriscontrol PUBLIC cxx_std_17)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt::DBus kdeconnectcore)
endif()
