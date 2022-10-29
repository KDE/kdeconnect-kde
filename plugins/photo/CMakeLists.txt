set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_photo_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PHOTO CATEGORY_NAME kdeconnect.plugin.photo
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin photo)")

set(kdeconnect_photo_SRCS
    photoplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_photo SOURCES ${kdeconnect_photo_SRCS})

target_link_libraries(kdeconnect_photo
    kdeconnectcore
    Qt::DBus
)
