set(smshelper_debug_file_SRCS)
ecm_qt_declare_logging_category(
    smshelper_debug_file_SRCS HEADER smshelper_debug.h
    IDENTIFIER KDECONNECT_SMS_SMSHELPER CATEGORY_NAME kdeconnect.sms.smshelper
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (smshelper)")

set(sms_debug_files_SRCS)
ecm_qt_declare_logging_category(
    sms_debug_files_SRCS HEADER sms_conversation_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATION_MODEL CATEGORY_NAME kdeconnect.sms.conversation
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversation model)")

ecm_qt_declare_logging_category(
    sms_debug_files_SRCS HEADER sms_conversations_list_debug.h
    IDENTIFIER KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL CATEGORY_NAME kdeconnect.sms.conversations_list
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (sms conversations list)")

add_library(kdeconnectsmshelper STATIC
    smshelper.cpp
    gsmasciimap.cpp
    ${smshelper_debug_file_SRCS}
)

target_include_directories(kdeconnectsmshelper PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(kdeconnectsmshelper
    PUBLIC
    Qt::Core
    Qt::DBus
    KF${QT_MAJOR_VERSION}::People
    Qt::Qml
    kdeconnectinterfaces
)

qt5_add_resources(kdeconnect_custom_icons_SRCS ${CMAKE_SOURCE_DIR}/icons/custom_icons.qrc)

add_executable(kdeconnect-sms
    main.cpp
    conversationlistmodel.cpp
    conversationmodel.cpp
    conversationssortfilterproxymodel.cpp
    resources.qrc
    thumbnailsprovider.cpp
    attachmentinfo.cpp
    ${kdeconnect_custom_icons_SRCS}
    ${sms_debug_files_SRCS})

target_link_libraries(kdeconnect-sms
    kdeconnectsmshelper
    kdeconnectinterfaces
    kdeconnectversion
    Qt::Quick
    Qt::QuickControls2
    Qt::Widgets
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::DBusAddons
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::People
    KF${QT_MAJOR_VERSION}::KCMUtils
    )

install(TARGETS kdeconnect-sms ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kdeconnect.sms.desktop DESTINATION ${KDE_INSTALL_APPDIR})
