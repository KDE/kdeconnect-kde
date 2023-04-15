set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_telephony_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_TELEPHONY CATEGORY_NAME kdeconnect.plugin.telephony
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin telephony)")

set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_telephony SOURCES ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::Notifications
    Qt::DBus
)
