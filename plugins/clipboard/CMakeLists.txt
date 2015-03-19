set(kdeconnect_clipboard_SRCS
    clipboardplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_clipboard JSON kdeconnect_clipboard.json SOURCES ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore KF5::Service Qt5::Gui)
