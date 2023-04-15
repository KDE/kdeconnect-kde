add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-plugins\")

add_subdirectory(ping)
add_subdirectory(connectivity-report)
add_subdirectory(remotecommands)
add_subdirectory(remotecontrol)
add_subdirectory(remotesystemvolume)
add_subdirectory(clipboard)
add_subdirectory(runcommand)


if(NOT WIN32)
    # https://bugs.kde.org/show_bug.cgi?id=444612
    add_subdirectory(battery)
endif()

if(NOT WIN32 AND NOT APPLE)
    add_subdirectory(bigscreen)
endif()

add_subdirectory(presenter)

if(NOT SAILFISHOS)
    if((WIN32 AND MSVC AND (${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 10.0.17763.0)) OR NOT WIN32)
        add_subdirectory(mpriscontrol)
    endif()
    add_subdirectory(photo)
    add_subdirectory(mprisremote)
    add_subdirectory(lockdevice)
    add_subdirectory(contacts)
    add_subdirectory(share)
    add_subdirectory(remotekeyboard)
    add_subdirectory(notifications)
    add_subdirectory(findmyphone)
    add_subdirectory(telephony)
    add_subdirectory(mousepad)
    add_subdirectory(sms)
    add_subdirectory(screensaver-inhibit)
    add_subdirectory(virtualmonitor)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        find_package(KF5ModemManagerQt ${KF5_MIN_VERSION})
        if (TARGET KF${QT_MAJOR_VERSION}::ModemManagerQt)
            add_subdirectory(mmtelephony)
        endif()
    endif()

    if(NOT WIN32)
        add_subdirectory(sendnotifications)
    endif()

    if(NOT APPLE)
        add_subdirectory(sftp)
    endif()

    if(KF5PulseAudioQt_FOUND OR WIN32)
        add_subdirectory(pausemusic)
    endif()

    if(Qt5Multimedia_FOUND AND (KF5PulseAudioQt_FOUND OR WIN32))
        add_subdirectory(findthisdevice)
    endif()

    if (WIN32 OR APPLE OR KF5PulseAudioQt_FOUND)
        add_subdirectory(systemvolume)
    endif()
endif()


# If we split notifications per plugin, in several notifyrc files, they won't
# appear in the same group in the Notifications KCM
install(FILES kdeconnect.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
