set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_virtualmonitor_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_VIRTUALMONITOR CATEGORY_NAME kdeconnect.plugin.virtualmonitor
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin virtualmonitor)")

set(kdeconnect_virtualmonitor_SRCS
    virtualmonitorplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_virtualmonitor
    SOURCES ${kdeconnect_virtualmonitor_SRCS})

target_link_libraries(kdeconnect_virtualmonitor
    kdeconnectcore
    Qt5::Core
    Qt5::Multimedia
    Qt5::DBus
)
