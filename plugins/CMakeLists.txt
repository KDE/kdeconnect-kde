include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/core)
add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-plugins\")
install(FILES kdeconnect_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

add_subdirectory(ping)
add_subdirectory(clipboard)
add_subdirectory(telephony)
add_subdirectory(share)
add_subdirectory(notifications)
add_subdirectory(battery)
add_subdirectory(runcommand)
if(NOT WIN32)
    add_subdirectory(pausemusic)
    add_subdirectory(mpriscontrol)
    add_subdirectory(mousepad)
    add_subdirectory(screensaver-inhibit)
    add_subdirectory(sftp)
endif()

add_subdirectory(findmyphone)

if(EXPERIMENTALAPP_ENABLED)
    add_subdirectory(mprisremote)
    add_subdirectory(remotecontrol)
    add_subdirectory(lockdevice)
endif()

#FIXME: If we split notifications in several files, they won't appear in the same group in the Notifications KCM
install(FILES kdeconnect.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
