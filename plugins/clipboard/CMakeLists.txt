find_package(KF5 ${KF5_MIN_VERSION} QUIET OPTIONAL_COMPONENTS Wayland)

kdeconnect_add_plugin(kdeconnect_clipboard JSON kdeconnect_clipboard.json SOURCES clipboardplugin.cpp clipboardlistener.cpp clipboardlistenerqt.cpp)

target_link_libraries(kdeconnect_clipboard kdeconnectcore Qt5::Gui)

if(TARGET KF5::WaylandClient)
    target_sources(kdeconnect_clipboard PRIVATE clipboardlistenerwayland.cpp)
    target_compile_definitions(kdeconnect_clipboard PRIVATE -DWITH_KWAYLAND)
    target_link_libraries(kdeconnect_clipboard KF5::WaylandClient)
endif()
