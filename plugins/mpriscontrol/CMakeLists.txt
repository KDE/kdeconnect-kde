if(WIN32)
    set(kdeconnect_mpriscontrol_SRCS
        mpriscontrolplugin-win.cpp
    )
elseif(APPLE)
    set(kdeconnect_mpriscontrol_SRCS
        mpriscontrolplugin-macos.mm
    )
else()
   set(kdeconnect_mpriscontrol_SRCS
       mpriscontrolplugin.cpp
   )
endif()

kdeconnect_add_plugin(kdeconnect_mpriscontrol SOURCES  ${kdeconnect_mpriscontrol_SRCS})

if(WIN32)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore windowsapp KF6::I18n)
elseif(APPLE)
   target_link_libraries(kdeconnect_mpriscontrol kdeconnectcore KF6::I18n "-framework AppKit" "-framework ApplicationServices")
else()
   target_link_libraries(kdeconnect_mpriscontrol Qt::DBus kdeconnectcore kdeconnectdbusinterfaces  kdeconnectmodels)
endif()
