set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_sms_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SMS CATEGORY_NAME kdeconnect.plugin.sms
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin sms)")

ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER kdeconnect_conversations_debug.h
    IDENTIFIER KDECONNECT_CONVERSATIONS CATEGORY_NAME kdeconnect.conversations
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (conversations)")

set(kdeconnect_sms_SRCS
    smsplugin.cpp
    conversationsdbusinterface.cpp
    requestconversationworker.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_sms SOURCES ${kdeconnect_sms_SRCS})

target_link_libraries(kdeconnect_sms
    kdeconnectcore
    kdeconnectinterfaces
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::Notifications
    Qt::Widgets
)
