set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
    clipboardlistener.cpp
)

kdeconnect_add_plugin(kdeconnect_clipboard JSON kdeconnect_clipboard.json SOURCES ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore Qt5::Gui)
