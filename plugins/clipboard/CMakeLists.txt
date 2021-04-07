if(UNIX AND NOT APPLE)
    find_package(QtWaylandScanner REQUIRED)
    find_package(Wayland 1.15 COMPONENTS Client)
    find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS WaylandClient)
endif()

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_clipboard_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_CLIPBOARD CATEGORY_NAME kdeconnect.plugin.clipboard
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin clipboard)")

set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
    clipboardlistener.cpp
    ${debug_file_SRCS}
)

if(UNIX AND NOT APPLE)
    list(APPEND kdeconnect_clipboard_SRCS datacontrol.cpp)
    ecm_add_qtwayland_client_protocol(kdeconnect_clipboard_SRCS
        PROTOCOL  wlr-data-control-unstable-v1.xml
        BASENAME wlr-data-control-unstable-v1
    )
    set(kdeconnect_clipboard_WL_LINK_LIBS Qt5::GuiPrivate # for native interface to get wl_seat
    Wayland::Client Qt::WaylandClient)
endif()


kdeconnect_add_plugin(kdeconnect_clipboard JSON kdeconnect_clipboard.json SOURCES ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore
                                    Qt5::Gui
                                    ${kdeconnect_clipboard_WL_LINK_LIBS}
                                    )
