add_library(kdeconnectsmshelper STATIC
    smshelper.cpp
    gsmasciimap.cpp
)

ecm_qt_declare_logging_category(kdeconnectsmshelper
    HEADER smshelper_debug.h
    IDENTIFIER KDECONNECT_SMS_SMSHELPER CATEGORY_NAME kdeconnect.sms.smshelper
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (smshelper)")
ecm_qt_declare_logging_category(kdeconnectsmshelper
    HEADER sms_conversation_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATION_MODEL CATEGORY_NAME kdeconnect.sms.conversation
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversation model)")
ecm_qt_declare_logging_category(kdeconnectsmshelper
    HEADER sms_conversations_list_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL CATEGORY_NAME kdeconnect.sms.conversations_list
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversations list)")


target_include_directories(kdeconnectsmshelper PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(kdeconnectsmshelper
    PUBLIC
    Qt::Core
    Qt::DBus
    KF6::People
    Qt::Qml
    kdeconnectinterfaces
)

qt_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-sms
    main.cpp
    conversationlistmodel.cpp
    conversationmodel.cpp
    conversationssortfilterproxymodel.cpp
    thumbnailsprovider.cpp
    attachmentinfo.cpp
    attachmentshelper.cpp
    ${kdeconnect_custom_icons_SRCS}
)

ecm_add_qml_module(kdeconnect-sms URI org.kde.kdeconnect.sms)

ecm_target_qml_sources(kdeconnect-sms SOURCES
    qml/Main.qml
    qml/ConversationList.qml
    qml/ConversationDisplay.qml
    qml/ChatMessage.qml
    qml/MessageAttachments.qml
    qml/AttachmentViewer.qml
    qml/SendingArea.qml
)

target_link_libraries(kdeconnect-sms PRIVATE
    kdeconnectsmshelper
    kdeconnectinterfaces
    kdeconnectversion
    Qt::Quick
    Qt::QuickControls2
    Qt::Widgets
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::People
    KF6::ColorScheme
    KF6::Crash
    )

install(TARGETS kdeconnect-sms ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
