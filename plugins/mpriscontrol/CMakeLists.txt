if(WIN32)
    set(kdeconnect_mpriscontrol_SRCS
        mpriscontrolplugin-win.cpp
    )
else()
   set(kdeconnect_mpriscontrol_SRCS
       mpriscontrolplugin.cpp
   )
endif()

kdeconnect_add_plugin(kdeconnect_mpriscontrol SOURCES  ${kdeconnect_mpriscontrol_SRCS})

ecm_qt_declare_logging_category(kdeconnect_mpriscontrol
    HEADER plugin_mpris_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_MPRIS CATEGORY_NAME kdeconnect.plugin.mpris
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin mpris)")

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore windowsapp)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt::DBus kdeconnectcore kdeconnectinterfaces)
endif()
