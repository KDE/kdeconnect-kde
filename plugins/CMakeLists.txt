include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/core)
add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-plugins\")
install(FILES kdeconnect_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

add_subdirectory(ping)
add_subdirectory(battery)
add_subdirectory(sendnotifications)
add_subdirectory(clipboard)

if(NOT SAILFISHOS)
    add_subdirectory(contacts)
    add_subdirectory(share)
    add_subdirectory(remotekeyboard)
    add_subdirectory(notifications)
    add_subdirectory(findmyphone)
    add_subdirectory(telephony)
    add_subdirectory(mousepad)
    if(NOT WIN32)
        add_subdirectory(runcommand)
        add_subdirectory(pausemusic)
        add_subdirectory(mpriscontrol)
        add_subdirectory(screensaver-inhibit)
        add_subdirectory(sftp)
    endif()

    if(Phonon4Qt5_FOUND)
        add_subdirectory(findthisdevice)
    endif()
endif()

if(SAILFISHOS OR EXPERIMENTALAPP_ENABLED)
    add_subdirectory(remotecommands)
    add_subdirectory(mprisremote)
    add_subdirectory(remotecontrol)
    add_subdirectory(lockdevice)
endif()

if(KF5PulseAudioQt_FOUND)
    add_subdirectory(systemvolume)
endif()


#FIXME: If we split notifications in several files, they won't appear in the same group in the Notifications KCM
install(FILES kdeconnect.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
