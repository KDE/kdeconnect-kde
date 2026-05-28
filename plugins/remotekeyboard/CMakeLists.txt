kdeconnect_add_plugin(kdeconnect_remotekeyboard SOURCES remotekeyboardplugin.cpp virtualkeyboardinjector.cpp)
target_link_libraries(kdeconnect_remotekeyboard
    kdeconnectcore
    Qt::DBus
)

# Virtual keyboard protocol (wlroots-based compositors like Hyprland/Sway)
find_package(WaylandScanner QUIET)
if(WaylandScanner_FOUND)
    find_package(Wayland QUIET COMPONENTS Client)
    if(Wayland_FOUND)
        # Copy the protocol XML from the mousepad plugin
        # The protocol XML is maintained in the mousepad plugin directory
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../mousepad/protocols/virtual-keyboard-unstable-v1.xml)
            configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/../mousepad/protocols/virtual-keyboard-unstable-v1.xml
                ${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1.xml
                COPYONLY
            )
            ecm_add_wayland_client_protocol(kdeconnect_remotekeyboard
                PROTOCOL ${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1.xml
                BASENAME virtual-keyboard
                PRIVATE_CODE
            )
            target_include_directories(kdeconnect_remotekeyboard PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
            target_link_libraries(kdeconnect_remotekeyboard Wayland::Client PkgConfig::XkbCommon)
        endif()
    endif()
endif()
