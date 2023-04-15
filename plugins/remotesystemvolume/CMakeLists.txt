set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_remotesystemvolume_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_REMOTESYSTEMVOLUME CATEGORY_NAME kdeconnect.plugin.remotesystemvolume
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin remotesystemvolume)")

set(kdeconnect_remotesystemvolume_SRCS
    remotesystemvolumeplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_remotesystemvolume SOURCES ${kdeconnect_remotesystemvolume_SRCS})

target_link_libraries(kdeconnect_remotesystemvolume
    kdeconnectcore
    Qt${QT_MAJOR_VERSION}::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
