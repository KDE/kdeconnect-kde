
install(FILES kdeconnect_plugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

add_subdirectory(ping)
add_subdirectory(pausemusic)
add_subdirectory(mpriscontrol)
add_subdirectory(clipboard)
add_subdirectory(telephony)
add_subdirectory(battery)
add_subdirectory(share)
add_subdirectory(notifications)
add_subdirectory(sftp)

