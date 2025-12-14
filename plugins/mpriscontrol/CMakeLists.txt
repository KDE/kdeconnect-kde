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

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore windowsapp KF6::I18n)
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt::DBus kdeconnectcore kdeconnectdbusinterfaces  kdeconnectmodels)
endif()
