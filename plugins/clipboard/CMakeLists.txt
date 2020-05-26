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

kdeconnect_add_plugin(kdeconnect_clipboard JSON kdeconnect_clipboard.json SOURCES ${kdeconnect_clipboard_SRCS})

target_link_libraries(kdeconnect_clipboard kdeconnectcore Qt5::Gui)
