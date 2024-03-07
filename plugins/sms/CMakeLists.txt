kdeconnect_add_plugin(kdeconnect_sms SOURCES ${kdeconnect_sms_SRCS})

target_sources(kdeconnect_sms PRIVATE
    smsplugin.cpp
    conversationsdbusinterface.cpp
    requestconversationworker.cpp
)
ecm_qt_declare_logging_category(kdeconnect_sms
    HEADER kdeconnect_conversations_debug.h
    IDENTIFIER KDECONNECT_CONVERSATIONS CATEGORY_NAME kdeconnect.conversations
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (conversations)")

find_package(Qt6 REQUIRED COMPONENTS Core5Compat)

target_link_libraries(kdeconnect_sms
    kdeconnectcore
    kdeconnectinterfaces
    Qt::DBus
    KF6::I18n
    KF6::Notifications
    Qt::Widgets
)
